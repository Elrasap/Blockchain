#pragma once
#include <vector>
#include <string>
#include "cluster/cluster_node.hpp"

class ClusterManager {
public:
    ClusterManager(int size, const std::string& version);
    void simulateTicks(int cycles);
    void rollingUpgrade(const std::string& newVersion);
private:
    std::vector<ClusterNode> nodes;
    void printStatus() const;
};

