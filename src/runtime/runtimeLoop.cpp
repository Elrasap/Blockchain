#include "runtime/runtime_loop.hpp"
#include <thread>
#include <chrono>
#include <iostream>

RuntimeLoop::RuntimeLoop(const std::string& releaseDir)
    : monitor(), upgrader(releaseDir) {}

void RuntimeLoop::run() {
    std::cout << "[Runtime] Node starting\n";
    for (int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        monitor.tick();

        if (i % 10 == 0) {
            std::cout << "[Runtime] Block committed\n";
            monitor.recordBlockCommit();
        }

        if (!monitor.isHealthy() && monitor.shouldRestart()) {
            std::cout << "[Runtime] Node unhealthy – restarting...\n";
            monitor = NodeHealthMonitor();
        }

        if (i % 20 == 0 && upgrader.checkForNewVersion()) {
            std::cout << "[Runtime] Upgrade detected\n";
            upgrader.downloadNewVersion();
            upgrader.applyUpgrade();
        }
    }
    std::cout << "[Runtime] Loop ended\n";
}

