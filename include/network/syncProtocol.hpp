#pragma once
#include <cstdint>

class PeerManager;

class SyncProtocol {
public:
    SyncProtocol(PeerManager& p);

    void requestPeerHeights();
    void syncMissingBlocks(uint64_t localHeight);

private:
    PeerManager& peers;
};

