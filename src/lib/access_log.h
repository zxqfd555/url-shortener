#pragma once

#include "util.h"
#include "manager.h"

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

    std::map<std::string, TAccessLogEntry> LastVisitEntry;
    std::mutex Lock;

public:
    void RegisterVisit (const std::string& slug, const uint32_t timestamp, const uint32_t ttl);
    void Clear ();
    void ExtendVisitedLinksLifetimes (const TBasicShortLinkManager& manager);
};

