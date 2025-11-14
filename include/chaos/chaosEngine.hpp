#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <map>
#include "chaos/chaos_fault.hpp"

struct ChaosMetrics {
    int crashes = 0;
    double maxLatencyMs = 0.0;
    double packetLossPct = 0.0;
    int peerDrops = 0;
    std::chrono::milliseconds finalityLag{0};
    std::chrono::milliseconds rto{0};
};

class ChaosEngine {
public:
    ChaosEngine(int clusterSize);
    void reset();
    void applyFault(const ChaosFault& f);
    void tick(std::chrono::milliseconds dt);
    ChaosMetrics snapshot() const;
    bool isClusterHealthy() const;
private:
    int size;
    std::vector<bool> alive;
    double baseLatencyMs = 250.0;
    double lossPct = 0.0;
    double cpuLoadPct = 5.0;
    bool diskError = false;
    int peers = 3;
    std::chrono::milliseconds sinceLastCommit{0};
    std::chrono::milliseconds timeToRecover{0};
    std::chrono::milliseconds finalityLag{0};
    int crashCount = 0;
    int peerDrops = 0;
    double maxLatency = 0.0;
};

