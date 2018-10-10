#include "slug.h"
#include "config.h"

bool TRandomMongoAwareSlugGenerator::HasLinkWithSlug (const std::string& slug) const {
    auto collection = (*MongoClient)[URL_SHORTENER_DB][URL_SLUG_COLLECTION];
    bsoncxx::stdx::optional<bsoncxx::document::value> maybeResult = collection.find_one(bsoncxx::builder::stream::document{} << "slug" << slug << bsoncxx::builder::stream::finalize);
    return !!maybeResult;
}

TRandomMongoAwareSlugGenerator::TRandomMongoAwareSlugGenerator(std::shared_ptr<mongocxx::instance> mongoInstance, std::shared_ptr<mongocxx::client> mongoClient)
    : MongoInstance(mongoInstance)
    , MongoClient(mongoClient)
{
}

std::string TRandomMongoAwareSlugGenerator::GenerateNewSlug () const {
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
    while (HasLinkWithSlug(slug));

    return slug;
}

std::string TSequentialSlugGenerator::GenerateNewSlug () const {
    std::lock_guard<std::mutex> g(Lock);
    Counter += 1;
    return std::to_string(Counter);
}

