#include "node/lifecycle_manager.hpp"
#include "core/logger.hpp"
#include "obs/health_checker.hpp"

LifecycleManager& LifecycleManager::instance() {
    static LifecycleManager inst;
    return inst;
}

void LifecycleManager::registerService(const std::string& name, std::function<bool()> healthFn, std::function<void()> restartFn) {
    services.push_back({name, healthFn, restartFn});
}

void LifecycleManager::startMonitor(int intervalMs) {
    if (running) return;
    running = true;
    monitor = std::thread([this, intervalMs]() {
        run(intervalMs);
    });
}

void LifecycleManager::run(int intervalMs) {
    while (running) {
        bool allHealthy = true;
        for (auto& s : services) {
            if (!s.health()) {
                Logger::instance().log(LogLevel::WARN, s.name + " unhealthy, restarting");
                s.restart();
                allHealthy = false;
            }
        }
        if (allHealthy) {
            HealthChecker::instance().markHealthy();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

void LifecycleManager::stopMonitor() {
    if (!running) return;
    running = false;
    if (monitor.joinable()) monitor.join();
}

