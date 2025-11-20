#pragma once

#include <cstdint>
#include "core/blockchain.hpp"
#include "core/mempool.hpp"
#include "network/peerManager.hpp"
#include "thirdparty/httplib.h"

namespace dnd {
class DndTxValidator;
}

class GossipServer {
public:
    GossipServer(int port,
                 Blockchain& chain,
                 Mempool& mempool,
                 PeerManager* peers = nullptr,
                 dnd::DndTxValidator* validator = nullptr);

    void start();
    void stop();

private:
    int port_;
    Blockchain& chain_;
    Mempool& mempool_;
    PeerManager* peers_;
    dnd::DndTxValidator* validator_;

    httplib::Server server;

    bool running = false;
};

