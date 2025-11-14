#pragma once
#include <string>
#include <map>
#include <chrono>

class MetricsCollector {
public:
    MetricsCollector(int nodeId);
    void recordBlockTime(double seconds);
    void recordPeerCount(int peers);
    void recordCpuLoad(double load);
    std::map<std::string,double> exportMetrics() const;
private:
    int id;
    double lastBlockTime;
    double cpuLoad;
    int peerCount;
    std::chrono::steady_clock::time_point lastUpdate;
};

