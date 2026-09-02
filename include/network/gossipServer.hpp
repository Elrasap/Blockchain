#pragma once

#include <cstdint>
#include "core/blockchain.hpp"
#include "core/mempool.hpp"
#include "network/peerManager.hpp"
#include "thirdparty/httplib.h"

class GossipServer {
public:
    GossipServer(int port,
                 Blockchain& chain,
                 Mempool& mempool,
                 PeerManager* peers = nullptr);

    void start();
    void stop();

private:
    int port_;
    Blockchain& chain_;
    Mempool& mempool_;
    PeerManager* peers_;
    httplib::Server server;

    bool running = false;
};
