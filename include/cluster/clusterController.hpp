#pragma once
#include <vector>
#include <string>
#include "node/node_instance.hpp"
#include "core/logger.hpp"
#include "obs/metrics.hpp"

class ClusterController {
public:
    ClusterController(int nodeCount);
    void startCluster();
    void rollingUpgrade();
    bool validateClusterState() const;
    void stopCluster();
private:
    std::vector<NodeInstance> nodes;
};

