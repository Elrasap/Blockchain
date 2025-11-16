#pragma once
#include <string>
#include <vector>
#include "core/block.hpp"

class PeerManager;

class SyncProtocol {
public:
    SyncProtocol(PeerManager& peers);

    // Ask peers for their height
    void requestPeerHeights();

    // Download blocks we miss
    void syncMissingBlocks(uint64_t localHeight);

private:
    PeerManager& peers;
};

