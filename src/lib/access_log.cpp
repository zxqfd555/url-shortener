#include "access_log.h"

void TAccessLog::RegisterVisit (const std::string& slug, const uint32_t timestamp, const uint32_t ttl) {
    std::lock_guard<std::mutex> g(Lock);
    LastVisitEntry[slug] = TAccessLogEntry(slug, timestamp, ttl);
}

void TAccessLog::Clear () {
    std::lock_guard<std::mutex> g(Lock);
    LastVisitEntry.clear();
}

void TAccessLog::ExtendVisitedLinksLifetimes (const TBasicShortLinkManager& manager) {
    std::map<std::string, TAccessLogEntry> ProcessableEntries;
    {
        CROW_LOG_INFO << "Before acquire mutex";
        std::lock_guard<std::mutex> g(Lock);
        CROW_LOG_INFO << "After acquire mutex";
        ProcessableEntries = std::move(LastVisitEntry);
        CROW_LOG_INFO << "After move";
        LastVisitEntry.clear();
        CROW_LOG_INFO << "After clear";
    }

    CROW_LOG_INFO << "Before iterating";
    for (auto&& entryIt : ProcessableEntries) {
        CROW_LOG_INFO << "Before extend";
        manager.ExtendLinkLifetime(entryIt.second.Slug, entryIt.second.Timestamp + entryIt.second.LinkTTL);
        CROW_LOG_INFO << "After extend";
    }
}

