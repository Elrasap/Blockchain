#pragma once
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include "core/logger.hpp"
#include "obs/healthChecker.hpp"

class LifecycleManager {
public:
    static LifecycleManager& instance();
    void registerService(const std::string& name, std::function<bool()> healthFn, std::function<void()> restartFn);
    void startMonitor(int intervalMs = 1000);
    void stopMonitor();
private:
    LifecycleManager() = default;
    std::thread monitor;
    std::atomic<bool> running{false};
    struct Entry { std::string name; std::function<bool()> health; std::function<void()> restart; };
    std::vector<Entry> services;
    void run(int intervalMs);
};

