// include/network/gossipServer.hpp
#pragma once

#include <cstdint>

#include "core/blockchain.hpp"
#include "core/mempool.hpp"
#include "network/peerManager.hpp"

namespace dnd {
    class DndTxValidator; // Vorwärtsdeklaration – volle Definition im .cpp
}

/**
 * GossipServer
 *
 * HTTP-Gossip-Endpunkte:
 *   - POST /gossip/tx
 *   - POST /gossip/block
 *
 * Verantwortlichkeiten:
 *   - nimmt signierte Transactions entgegen
 *   - validiert + legt in Mempool
 *   - broadcastet TX/Blocks an Peers
 *   - kann reine DnD-Event-JSONs in eine Transaction "wrappen"
 */
class GossipServer {
public:
    GossipServer(int port,
                 Blockchain& chain,
                 Mempool& mempool,
                 PeerManager* peers = nullptr,
                 dnd::DndTxValidator* validator = nullptr);

    // Blockiert – startet HTTP-Server (httplib::Server::listen)
    void start();

private:
    int port_;
    Blockchain& chain_;
    Mempool& mempool_;
    PeerManager* peers_;
    dnd::DndTxValidator* validator_;
};

