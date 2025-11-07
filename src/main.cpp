#include <iostream>
#include <thread>
#include <chrono>
#include "obs/metrics.hpp"
#include "obs/metricsServer.hpp"
#include "obs/healthChecker.hpp"
#include "core/logger.hpp"

int main() {
    Logger::instance().log(LogLevel::INFO, "Cluster Dashboard & Alerts v1.2.0 starting");

    MetricsServer server(9100);
    server.start();

    HealthChecker::instance().setBlockHeight(42);
    HealthChecker::instance().setPeerCount(3);

    for (int i = 0; i < 5; ++i) {
        Metrics::instance().incCounter("blocks_committed_total");
        Metrics::instance().observe("block_commit_seconds", 0.2 + 0.1 * i);
        if (0.2 + 0.1 * i > 0.5)
            HealthChecker::instance().markUnhealthy("slow block commits");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    Logger::instance().log(LogLevel::INFO, "HTTP server listening on port 9100");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    server.stop();
    Logger::instance().log(LogLevel::INFO, "Server stopped");
    return 0;
}

