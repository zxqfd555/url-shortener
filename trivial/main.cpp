#include <ctime>
#include <iostream>
#include <unordered_set>
#include <thread>

#include "crow.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include "config.h"
#include "slug.h"
#include "../common/manager.h"

uint32_t GetCurrentTimestamp() {
    std::time_t result = std::time(nullptr);
    return (uint32_t)(result);
}

class TBasicShortLinkManager : public IShortLinkManager {
private:
    std::shared_ptr<mongocxx::instance> MongoInstance;
    std::shared_ptr<mongocxx::client> MongoClient;
    std::unique_ptr<ISlugGenerator> SlugGenerator;

public:
    TBasicShortLinkManager(std::shared_ptr<mongocxx::instance> mongoInstance, std::shared_ptr<mongocxx::client> mongoClient, std::unique_ptr<ISlugGenerator>&& slugGenerator)
        : MongoInstance(mongoInstance)
        , MongoClient(mongoClient)
        , SlugGenerator(std::move(slugGenerator))
    {
    }

    virtual std::string AddLink (const std::string& originalUrl, const uint32_t ttl) const override final {
        std::string slug = SlugGenerator->GenerateNewSlug();

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
    srand(time(nullptr));
    crow::SimpleApp app;

    /*
        Initialize all of the required stuff.

        Initialize Mongo Instance and Client.
    */

    std::shared_ptr<mongocxx::instance> MongoInstance = std::shared_ptr<mongocxx::instance>(new mongocxx::instance{});
    std::shared_ptr<mongocxx::client> MongoClient = std::shared_ptr<mongocxx::client>(new mongocxx::client{mongocxx::uri{}});

    /*
        Create slug generator according to the config.
    */
    std::unique_ptr<ISlugGenerator> slugGenerator;
    if (SLUG_GENERATOR_TYPE == ESlugGeneratorType::RandomMongoAware) {
        slugGenerator = std::unique_ptr<ISlugGenerator>(new TRandomMongoAwareSlugGenerator(MongoInstance, MongoClient));
    } else if (SLUG_GENERATOR_TYPE == ESlugGeneratorType::Sequential) {
        slugGenerator = std::unique_ptr<ISlugGenerator>(new TSequentialSlugGenerator());
    }

    /*
        Create Link Manager with the chosen link generator.
    */
    TBasicShortLinkManager Manager(MongoInstance, MongoClient, std::move(slugGenerator));

    /*
        Start link lifetime watching daemon.
    */
    TLinkActualizerDaemon Daemon(Manager, AccessLog, DAEMON_LAUNCH_FREQUENCY);
    Daemon.Start();

    /*
        Everything is done in the initialization now. Create endpoints.

        Link posting endpoint.
    */
    CROW_ROUTE(app, "/link")
        .methods("POST"_method)
        ([&Manager] (const crow::request& req) {
            auto requestJson = crow::json::load(req.body);
            if (!requestJson) {
                return crow::response(400);
            }

            /*
                Shortening happens here.
            */
            std::string shortenedLinkSlug = Manager.AddLink(requestJson["original_url"].s(), requestJson["ttl"].i());

            /*
                201 Created.
                The path to the shortened link shall be passed through Location header.
            */
            crow::response response(201);
            response.add_header("Location", BASE_HOST + shortenedLinkSlug);

            return response;
        });

    /*
        Follow-by-link endpoint.
    */
    CROW_ROUTE(app, "/<string>")
        .methods("GET"_method)
        ([&Manager] (std::string slug) {
            crow::response response;

            TShortLinkRecord result;
            bool success = Manager.GetOriginalLink(slug, &result);
            if (success && result.ExpirationTimestamp >= GetCurrentTimestamp()) {
                /*
                    If the link exists, write it in the location header.
                */
                response.add_header("Location", result.OriginalUrl);
                response.code = 302;

                /*
                    Extend the lifetime by a direct DB write if it is going to expire soon.
                */
                if (result.ExpirationTimestamp - GetCurrentTimestamp() < DAEMON_LAUNCH_FREQUENCY + DAEMON_ITERATION_DURATION) {
                    Manager.ExtendLinkLifetime(slug, GetCurrentTimestamp() + result.TTL);
                }

                /*
                    Write an access log entry, so that we can extend non-urgent links in the regular process, later.
                */
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

