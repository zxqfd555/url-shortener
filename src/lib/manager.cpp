#include "manager.h"
#include "config.h"
#include "util.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

TBasicShortLinkManager::TBasicShortLinkManager(std::shared_ptr<mongocxx::instance> mongoInstance, std::shared_ptr<mongocxx::pool> mongoClientPool, std::unique_ptr<ISlugGenerator>&& slugGenerator)
    : MongoInstance(mongoInstance)
    , MongoClientPool(mongoClientPool)
    , SlugGenerator(std::move(slugGenerator))
{
}

std::string TBasicShortLinkManager::AddLink (const std::string& originalUrl, const uint32_t ttl) const {
    std::string slug = SlugGenerator->GenerateNewSlug();

    auto client = MongoClientPool->acquire();
    auto collection = (*client)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
    bsoncxx::builder::stream::document document{};
    document << "original_url" << originalUrl;
    document << "slug" << slug;
    document << "ttl" << (int64_t)ttl;
    document << "expiration_timestamp" << (int64_t)(GetCurrentTimestamp() + ttl);
    collection.insert_one(document.view());
        
    return slug;
}

bool TBasicShortLinkManager::GetOriginalLink (const std::string& slug, TShortLinkRecord* result) const {
    auto client = MongoClientPool->acquire();
    auto collection = (*client)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
    bsoncxx::stdx::optional<bsoncxx::document::value> maybeResult;
    try {
        maybeResult = collection.find_one(bsoncxx::builder::stream::document{} << "slug" << slug << bsoncxx::builder::stream::finalize);
    } catch (std::exception e) {
        return false;
    }

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

void TBasicShortLinkManager::ExtendLinkLifetime (const std::string& slug, const uint32_t newExpirationTimestamp) const {
    auto client = MongoClientPool->acquire();
    auto collection = (*client)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
    try {
        collection.update_one(
            bsoncxx::builder::stream::document{} << "slug" << slug << bsoncxx::builder::stream::finalize,
            bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document <<
            "expiration_timestamp" << (int64_t)newExpirationTimestamp << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize
        );
    } catch (std::exception e) {
        CROW_LOG_ERROR << "Exception in extend lifetime";
    }
}

void TBasicShortLinkManager::BulkExtendLifetime (const std::vector<std::pair<std::string, std::uint32_t>>&& items) const {
    if (items.empty()) {
        return;
    }

    auto client = MongoClientPool->acquire();
    auto collection = (*client)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];

    auto bulk = collection.create_bulk_write();
    for (auto&& extendInfo : items) {
        auto document = make_document(kvp("slug", extendInfo.first));
        auto updateAction = make_document(kvp("$set", make_document(kvp("expiration_timestamp", (int64_t)extendInfo.second))));
        mongocxx::model::update_one upsertOperation{document.view(), updateAction.view()};
        bulk.append(upsertOperation);
    }

    auto result = bulk.execute();
    if (!result) {
        CROW_LOG_CRITICAL << "Bulk update failed!";
    }
}

void TBasicShortLinkManager::DeleteLink (const std::string& slug) const {
    auto client = MongoClientPool->acquire();
    auto collection = (*client)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
    collection.delete_one(bsoncxx::builder::stream::document{} << "slug" << slug << bsoncxx::builder::stream::finalize);
}

void TBasicShortLinkManager::DeleteExpiredLinks () const {
    auto client = MongoClientPool->acquire();
    auto collection = (*client)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
    try {
        collection.delete_many(
            bsoncxx::builder::stream::document{} << "expiration_timestamp" << bsoncxx::builder::stream::open_document <<
            "$lte" << (int64_t)GetCurrentTimestamp() << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize
        );
    } catch (std::exception e) {
        CROW_LOG_ERROR << "Exception in bulk delete";
    }
}

