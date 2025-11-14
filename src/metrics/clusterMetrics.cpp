#include "metrics/cluster_metrics.hpp"
#include <random>

ClusterMetrics::ClusterMetrics(int clusterSize) {
    for (int i = 0; i < clusterSize; ++i)
        collectors.emplace_back(i);
}

void ClusterMetrics::simulateUpdate() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> timeDist(0.2, 1.2);
    std::uniform_real_distribution<> cpuDist(5.0, 70.0);
    std::uniform_int_distribution<> peerDist(1, 5);

    for (auto& c : collectors) {
        c.recordBlockTime(timeDist(gen));
        c.recordCpuLoad(cpuDist(gen));
        c.recordPeerCount(peerDist(gen));
    }
}

std::map<std::string,double> ClusterMetrics::aggregate() const {
    double avgBlock = 0.0, avgCpu = 0.0;
    double totalPeers = 0.0;
    for (const auto& c : collectors) {
        auto m = c.exportMetrics();
        avgBlock += m.at("last_block_time_s");
        avgCpu += m.at("cpu_load");
        totalPeers += m.at("peer_count");
    }
    int n = collectors.size();
    return {
        {"avg_block_time_s", avgBlock / n},
        {"avg_cpu_load", avgCpu / n},
        {"total_peers", totalPeers}
    };
}

