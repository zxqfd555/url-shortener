#include <ctime>
#include <iostream>
#include <thread>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>

#include "lib/access_log.h"
#include "lib/config.h"
#include "lib/crow.h"
#include "lib/daemon.h"
#include "lib/manager.h"
#include "lib/slug.h"
#include "lib/util.h"

TAccessLog AccessLog; // Can be large while that stack can be small.

int main() {
    srand(time(nullptr));
    crow::SimpleApp app;

    /*
        Initialize all of the required stuff.

        Initialize Mongo Instance and Client.
    */

    mongocxx::uri uri{"mongodb://localhost:27017/?minPoolSize=128&maxPoolSize=128"};
    std::shared_ptr<mongocxx::instance> MongoInstance = std::shared_ptr<mongocxx::instance>(new mongocxx::instance{});   
    std::shared_ptr<mongocxx::pool> MongoClientPool = std::shared_ptr<mongocxx::pool>(new mongocxx::pool{uri});

    /*
        Create slug generator according to the config.
    */
    std::unique_ptr<ISlugGenerator> slugGenerator;
    if (SLUG_GENERATOR_TYPE == ESlugGeneratorType::RandomMongoAware) {
        slugGenerator = std::unique_ptr<ISlugGenerator>(new TRandomMongoAwareSlugGenerator(MongoInstance, MongoClientPool));
    } else if (SLUG_GENERATOR_TYPE == ESlugGeneratorType::Sequential) {
        slugGenerator = std::unique_ptr<ISlugGenerator>(new TSequentialSlugGenerator());
    }

    /*
        Create Link Manager with the chosen link generator.
    */
    TBasicShortLinkManager Manager(MongoInstance, MongoClientPool, std::move(slugGenerator));

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

    app.loglevel(crow::LogLevel::CRITICAL);
    app.port(DEPLOY_PORT)
        .multithreaded()
        .run();
}

