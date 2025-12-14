#pragma once
#include <string>
#include <vector>
#include "core/block.hpp"
#include "core/transaction.hpp"

class PeerManager;

class GossipProtocol {
public:
    GossipProtocol(PeerManager& peers);

    void gossipTransaction(const Transaction& tx);
    void gossipBlock(const Block& block);

private:
    PeerManager& peers;

    void sendToPeer(const std::string& address,
                    const std::string& path,
                    const std::string& json);
};

