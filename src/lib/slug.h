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

class ISlugGenerator {
public:
    virtual std::string GenerateNewSlug () const = 0;
};

class TRandomMongoAwareSlugGenerator : public ISlugGenerator {
private:
    std::shared_ptr<mongocxx::instance> MongoInstance;
    std::shared_ptr<mongocxx::client> MongoClient;
    bool HasLinkWithSlug (const std::string& slug) const;

public:
    TRandomMongoAwareSlugGenerator(std::shared_ptr<mongocxx::instance> mongoInstance, std::shared_ptr<mongocxx::client> mongoClient);
    virtual std::string GenerateNewSlug () const override;
};


class TSequentialSlugGenerator : public ISlugGenerator {
private:
    mutable uint32_t Counter = 0;
    mutable std::mutex Lock;

public:
    virtual std::string GenerateNewSlug () const override;
};

