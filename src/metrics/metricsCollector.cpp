#include "metrics/metricsCollector.hpp"
#include <random>
#include <chrono>

MetricsCollector::MetricsCollector(int nodeId)
    : id(nodeId), lastBlockTime(0.0), cpuLoad(0.0), peerCount(0),
      lastUpdate(std::chrono::steady_clock::now()) {}

void MetricsCollector::recordBlockTime(double seconds) {
    lastBlockTime = seconds;
    lastUpdate = std::chrono::steady_clock::now();
}

void MetricsCollector::recordPeerCount(int peers) {
    peerCount = peers;
    lastUpdate = std::chrono::steady_clock::now();
}

void MetricsCollector::recordCpuLoad(double load) {
    cpuLoad = load;
    lastUpdate = std::chrono::steady_clock::now();
}

std::map<std::string,double> MetricsCollector::exportMetrics() const {
    return {
        {"node_id", static_cast<double>(id)},
        {"last_block_time_s", lastBlockTime},
        {"cpu_load", cpuLoad},
        {"peer_count", static_cast<double>(peerCount)}
    };
}

