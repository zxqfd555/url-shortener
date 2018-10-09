#pragma once

#include <algorithm>
#include <cstring>

struct TShortLinkRecord {
    std::string Slug;
    std::string OriginalUrl;
    uint32_t ExpirationTimestamp;
    uint32_t TTL;    
};

class IShortLinkManager {
public:
    virtual std::string AddLink (const std::string& originalUrl, const uint32_t ttl) const = 0;
    virtual bool GetOriginalLink (const std::string& slug, TShortLinkRecord* result) const = 0;
    ~IShortLinkManager() {
    }
};

