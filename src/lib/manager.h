#pragma once

#include <ctime>
#include <iostream>
#include <unordered_set>
#include <thread>

#include "crow.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>

#include "slug.h"

struct TShortLinkRecord {
    std::string Slug;
    std::string OriginalUrl;
    uint32_t ExpirationTimestamp;
    uint32_t TTL;    
};

class TBasicShortLinkManager {
private:
    std::shared_ptr<mongocxx::instance> MongoInstance;
    std::shared_ptr<mongocxx::pool> MongoClientPool;
    std::unique_ptr<ISlugGenerator> SlugGenerator;

public:
    TBasicShortLinkManager(std::shared_ptr<mongocxx::instance> mongoInstance, std::shared_ptr<mongocxx::pool> mongoClientPool, std::unique_ptr<ISlugGenerator>&& slugGenerator);
    std::string AddLink (const std::string& originalUrl, const uint32_t ttl) const;
    bool GetOriginalLink (const std::string& slug, TShortLinkRecord* result) const;
    void ExtendLinkLifetime (const std::string& slug, const uint32_t newExpirationTimestamp) const;
    void DeleteLink (const std::string& slug) const;
    void DeleteExpiredLinks () const;
};

