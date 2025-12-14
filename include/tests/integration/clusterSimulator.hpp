#pragma once
#include <cstdint>
#include <vector>
#include "block.hpp"
#include "transaction.hpp"

class ClusterSimulator {
public:
    void startCluster(uint64_t n);
    void stopNode(uint64_t nodeId);
    void partition(Block* nodeA, Block* nodeB);
    void healAll();
    void broadcastTransaction(const Transaction& tx);
};

