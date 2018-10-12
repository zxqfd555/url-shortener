#include "daemon.h"

TLinkActualizerDaemon::TLinkActualizerDaemon(const TBasicShortLinkManager& linkManager, TAccessLog& accessLog, const uint32_t launchFrequency)
    : LinkManager(linkManager)
    , AccessLog(accessLog)
    , LaunchFrequency(launchFrequency)
{
}

void TLinkActualizerDaemon::ProcessLogEntries () {
    CROW_LOG_INFO << "Started LinkActualizerDaemon";
    while (true) {
        CROW_LOG_INFO << "Staring LinkActualizerDaemon iteration";

        uint32_t tsBeforeLaunch = GetCurrentTimestamp();
        AccessLog.ExtendVisitedLinksLifetimes(LinkManager);
        LinkManager.DeleteExpiredLinks();
        uint32_t timeTotalSpent = GetCurrentTimestamp() - tsBeforeLaunch;

        CROW_LOG_CRITICAL << "The iteration took " << timeTotalSpent << " seconds";
        if (timeTotalSpent < LaunchFrequency) {
            CROW_LOG_INFO << "The daemon thread goes to sleep for extra " << LaunchFrequency - timeTotalSpent << " seconds";
            std::this_thread::sleep_for(std::chrono::seconds(LaunchFrequency - timeTotalSpent));
        } else {
            CROW_LOG_CRITICAL << "Problem! The update was done in " << timeTotalSpent << " seconds, while we only have " << LaunchFrequency;
        }
    }
}

void TLinkActualizerDaemon::Start() {
    mainThread = std::unique_ptr<std::thread>(new std::thread(&TLinkActualizerDaemon::ProcessLogEntries, this));
}

