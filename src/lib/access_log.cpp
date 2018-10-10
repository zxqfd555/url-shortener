#include "access_log.h"

void TAccessLog::RegisterVisit (const std::string& slug, const uint32_t timestamp, const uint32_t ttl) {
    std::lock_guard<std::mutex> g(Lock);
    Entries.push_back(TAccessLogEntry(slug, timestamp, ttl));
}

void TAccessLog::Clear () {
    std::lock_guard<std::mutex> g(Lock);
    Entries.clear();
}

void TAccessLog::ExtendVisitedLinksLifetimes (const TBasicShortLinkManager& manager) {
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

