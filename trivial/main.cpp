#include <ctime>
#include <iostream>
#include <unordered_set>
#include <thread>

#include "crow.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include "../common/manager.h"

const std::string URL_SHORTENER_DB = "url_shortener";
const std::string URL_SLUG_COLLECTION = "slug";

const uint32_t SLUG_LENGTH = 6;

const uint32_t DAEMON_LAUNCH_FREQUENCY = 90;
const uint32_t DAEMON_ITERATION_DURATION = 30;

const uint32_t DEPLOY_PORT = 18080;


uint32_t GetCurrentTimestamp() {
    std::time_t result = std::time(nullptr);
    return (uint32_t)(result);
}


class TBasicShortLinkManager : public IShortLinkManager {
private:
    std::unique_ptr<mongocxx::instance> MongoInstance;
    std::unique_ptr<mongocxx::client> MongoClient;

    std::string GenerateNewSlug () const {
        std::string slug;

        do {
            slug = "";
            for (size_t i = 0; i < SLUG_LENGTH; ++i) {
                uint32_t currentCharIndex = rand() % 62;
                if (currentCharIndex < 26) {
                    slug += (char)('a' + currentCharIndex);
                } else if (currentCharIndex < 26 + 26) {
                    slug += (char)('A' + currentCharIndex - 26);
                } else {
                    slug += (char)('0' + currentCharIndex - 26 - 26);
                }
            }
        }
        while (GetOriginalLink(slug, nullptr));

        return slug;
    }

public:
    TBasicShortLinkManager() {
        MongoInstance = std::unique_ptr<mongocxx::instance>(new mongocxx::instance{});
        MongoClient = std::unique_ptr<mongocxx::client>(new mongocxx::client{mongocxx::uri{}});
    }

    virtual std::string AddLink (const std::string& originalUrl, const uint32_t ttl) const override final {
        std::string slug = GenerateNewSlug();

        auto collection = (*MongoClient)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
        bsoncxx::builder::stream::document document{};
        document << "original_url" << originalUrl;
        document << "slug" << slug;
        document << "ttl" << (int64_t)ttl;
        document << "expiration_timestamp" << (int64_t)(GetCurrentTimestamp() + ttl);
        collection.insert_one(document.view());
        
        return slug;
    }

    virtual bool GetOriginalLink (const std::string& slug, TShortLinkRecord* result) const override final {
        auto collection = (*MongoClient)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
        bsoncxx::stdx::optional<bsoncxx::document::value> maybeResult = collection.find_one(bsoncxx::builder::stream::document{} << "slug" << slug << bsoncxx::builder::stream::finalize);

        if (maybeResult) {
            if (result != nullptr) {
                bsoncxx::document::view view = maybeResult.value().view();
                result->Slug = slug;
                result->TTL = view.find("ttl")->get_int64().value;
                result->ExpirationTimestamp = view.find("expiration_timestamp")->get_int64().value;
                result->OriginalUrl = view.find("original_url")->get_utf8().value.to_string();
            }
            return true;
        }
        return false;
    }
    
    void ExtendLinkLifetime (const std::string& slug, const uint32_t newExpirationTimestamp) const {
        auto collection = (*MongoClient)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
        collection.update_one(
            bsoncxx::builder::stream::document{} << "slug" << slug << bsoncxx::builder::stream::finalize,
            bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document <<
            "expiration_timestamp" << (int64_t)newExpirationTimestamp << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize
        );
    }

    void DeleteLink (const std::string& slug) const {
        auto collection = (*MongoClient)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];    
        collection.delete_one(bsoncxx::builder::stream::document{} << "slug" << slug << bsoncxx::builder::stream::finalize);
    }

    void DeleteExpiredLinks () const {
        auto collection = (*MongoClient)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
        collection.delete_many(
            bsoncxx::builder::stream::document{} << "expiration_timestamp" << bsoncxx::builder::stream::open_document <<
            "$lte" << (int64_t)GetCurrentTimestamp() << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize
        );
    }
};

class TAccessLog {
private:

    struct TAccessLogEntry {
        std::string Slug;
        uint32_t Timestamp;
        uint32_t LinkTTL;

        TAccessLogEntry() = default;

        TAccessLogEntry(std::string slug, uint32_t timestamp, uint32_t linkTTL)
            : Slug(slug)
            , Timestamp(timestamp)
            , LinkTTL(linkTTL)
        {
        }
    };

    std::vector<TAccessLogEntry> Entries;
    std::mutex Lock;

public:
    void RegisterVisit (const std::string& slug, const uint32_t timestamp, const uint32_t ttl) {
        std::lock_guard<std::mutex> g(Lock);
        Entries.push_back(TAccessLogEntry(slug, timestamp, ttl));
    }

    void Clear () {
        std::lock_guard<std::mutex> g(Lock);
        Entries.clear();
    }

    void ExtendVisitedLinksLifetimes (const TBasicShortLinkManager& manager) {
        std::lock_guard<std::mutex> g(Lock);

        std::reverse(Entries.begin(), Entries.end());
        std::unordered_set<std::string> processedSlugs;
        for (auto&& entry : Entries) {
            if (processedSlugs.find(entry.Slug) != processedSlugs.end()) {
                continue;
            }
            manager.ExtendLinkLifetime(entry.Slug, entry.Timestamp + entry.LinkTTL);
            processedSlugs.insert(entry.Slug);
        }

        Entries.clear();
    }
};

class TLinkActualizerDaemon {
private:
    const TBasicShortLinkManager& LinkManager;
    const uint32_t LaunchFrequency;
    TAccessLog& AccessLog;

    std::unique_ptr<std::thread> mainThread;

    void ProcessLogEntries () {
        CROW_LOG_INFO << "Started LinkActualizerDaemon";
        while (true) {
            CROW_LOG_INFO << "Staring LinkActualizerDaemon iteration";

            uint32_t tsBeforeLaunch = GetCurrentTimestamp();
            AccessLog.ExtendVisitedLinksLifetimes(LinkManager);
            LinkManager.DeleteExpiredLinks();
            uint32_t timeTotalSpent = GetCurrentTimestamp() - tsBeforeLaunch;

            CROW_LOG_INFO << "The iteration took " << timeTotalSpent << " seconds";
            if (timeTotalSpent < LaunchFrequency) {
                CROW_LOG_INFO << "The daemon thread goes to sleep for extra " << LaunchFrequency - timeTotalSpent << " seconds";
                std::this_thread::sleep_for(std::chrono::seconds(LaunchFrequency - timeTotalSpent));
            } else {
                CROW_LOG_CRITICAL << "Problem! The update was done in " << timeTotalSpent << " seconds, while we only have " << LaunchFrequency;
            }
        }
    }

public:
    TLinkActualizerDaemon(const TBasicShortLinkManager& linkManager, TAccessLog& accessLog, const uint32_t launchFrequency)
        : LinkManager(linkManager)
        , AccessLog(accessLog)
        , LaunchFrequency(launchFrequency)
    {
    }

    void Start() {
        mainThread = std::unique_ptr<std::thread>(new std::thread(&TLinkActualizerDaemon::ProcessLogEntries, this));
    }
};

TAccessLog AccessLog; // Can be large while that stack can be small.

int main() {
    TBasicShortLinkManager Manager;
    TLinkActualizerDaemon Daemon(Manager, AccessLog, DAEMON_LAUNCH_FREQUENCY);
    Daemon.Start();

    srand(time(nullptr));
    crow::SimpleApp app;

    CROW_ROUTE(app, "/link")
        .methods("POST"_method)
        ([&Manager] (const crow::request& req) {
            auto requestJson = crow::json::load(req.body);
            if (!requestJson) {
                return crow::response(400);
            }

            std::string shortenedLinkSlug = Manager.AddLink(requestJson["original_url"].s(), requestJson["ttl"].i());

            crow::response response(201);
            response.add_header("Location", shortenedLinkSlug);

            return response;
        });

    CROW_ROUTE(app, "/<string>")
        .methods("GET"_method)
        ([&Manager] (std::string slug) {
            crow::response response;

            TShortLinkRecord result;
            bool success = Manager.GetOriginalLink(slug, &result);
            if (success && result.ExpirationTimestamp >= GetCurrentTimestamp()) {
                response.add_header("Location", result.OriginalUrl);
                response.code = 302;

                if (result.ExpirationTimestamp - GetCurrentTimestamp() < DAEMON_LAUNCH_FREQUENCY + DAEMON_ITERATION_DURATION) {
                    Manager.ExtendLinkLifetime(slug, GetCurrentTimestamp() + result.TTL);
                }

                AccessLog.RegisterVisit(slug, GetCurrentTimestamp(), result.TTL);
            } else {
                response.code = 404;
            }

            return response;
        });

    app.loglevel(crow::LogLevel::DEBUG);
    app.port(DEPLOY_PORT)
        .multithreaded()
        .run();
}

