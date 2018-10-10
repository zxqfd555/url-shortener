#pragma once

#include "crow.h"
#include "manager.h"
#include "access_log.h"

class TLinkActualizerDaemon {
private:
    const TBasicShortLinkManager& LinkManager;
    const uint32_t LaunchFrequency;
    TAccessLog& AccessLog;
    std::unique_ptr<std::thread> mainThread;

    void ProcessLogEntries ();

public:
    TLinkActualizerDaemon(const TBasicShortLinkManager& linkManager, TAccessLog& accessLog, const uint32_t launchFrequency);
    void Start();
};

