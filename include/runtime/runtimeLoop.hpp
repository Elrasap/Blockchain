#pragma once
#include <string>
#include "runtime/node_health_monitor.hpp"
#include "runtime/upgrade_manager.hpp"

class RuntimeLoop {
public:
    explicit RuntimeLoop(const std::string& releaseDir);
    void run();
private:
    NodeHealthMonitor monitor;
    UpgradeManager upgrader;
};

