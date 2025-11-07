#pragma once
#include <string>
#include <vector>
#include <memory>

class RollingUpgradeController {
public:
    void drainNode(uint64_t nodeId);        // stoppt neue TXs, wartet Commit ab
    void upgradeBinary(uint64_t nodeId);
    void rejoinCluster(uint64_t nodeId);
    bool verifyClusterHealth();

};
