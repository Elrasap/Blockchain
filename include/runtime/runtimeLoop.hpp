#pragma once
#include <string>
#include "runtime/nodeHealthMonitor.hpp"
#include "runtime/upgradeManager.hpp"

class RuntimeLoop {
public:
    explicit RuntimeLoop(const std::string& releaseDir);
    void run();
private:
    NodeHealthMonitor monitor;
    UpgradeManager upgrader;
};

