#pragma once
#include <string>
#include <map>
#include <chrono>

class MetricsCollector {
public:
    uint64_t network_peers_total;
    uint64_t network_tx_gossip_total;
    uint64_t network_block_gossip_total;

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

