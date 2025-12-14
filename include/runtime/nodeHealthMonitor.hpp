#pragma once
#include <string>
#include <chrono>

class NodeHealthMonitor {
public:
    NodeHealthMonitor();
    void recordBlockCommit();
    bool isHealthy() const;
    bool shouldRestart() const;
    void tick();
private:
    std::chrono::steady_clock::time_point lastCommit;
    int consecutiveFails;
};

