#pragma once
#include <cstdint>

#include "core/blockchain.hpp"
#include "core/mempool.hpp"
#include "network/peerManager.hpp"

namespace dnd {
    class DndTxValidator; // <-- nur Vorwärtsdeklaration
}

class GossipServer {
public:
    GossipServer(int port,
                 Blockchain& chain,
                 Mempool& mempool,
                 PeerManager* peers = nullptr,
                 dnd::DndTxValidator* validator = nullptr);

    void start();

private:
    int port_;
    Blockchain& chain_;
    Mempool& mempool_;
    PeerManager* peers_;
    dnd::DndTxValidator* validator_;
};

