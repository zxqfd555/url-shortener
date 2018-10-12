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
        std::lock_guard<std::mutex> g(Lock);
        ProcessableEntries = std::move(LastVisitEntry);
        LastVisitEntry.clear();
    }

    std::vector<std::pair<std::string, std::uint32_t>> updateBulk;
    for (auto&& entryIt : ProcessableEntries) {
        updateBulk.push_back(std::make_pair(entryIt.second.Slug, entryIt.second.Timestamp + entryIt.second.LinkTTL));
    }
    CROW_LOG_CRITICAL << "Update bulk size" << updateBulk.size();
    manager.BulkExtendLifetime(std::move(updateBulk));
}

