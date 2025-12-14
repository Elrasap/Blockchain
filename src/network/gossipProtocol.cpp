#include "network/gossipProtocol.hpp"
#include "network/peerManager.hpp"
#include <thirdparty/httplib.h>
#include <nlohmann/json.hpp>
#include "network/gossipProtocol.hpp"
#include "network/peerManager.hpp"
#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>

// dann gossipTransaction / gossipBlock implementieren

using json = nlohmann::json;

GossipProtocol::GossipProtocol(PeerManager& p)
    : peers(p) {}

void GossipProtocol::sendToPeer(const std::string& addr,
                                const std::string& path,
                                const std::string& body)
{
    httplib::Client cli(addr.c_str());
    cli.Post(path.c_str(), body, "application/json");
}

void GossipProtocol::gossipTransaction(const Transaction& tx) {
    json j;
    j["sender"] = tx.senderPubkey;
    j["payload"] = tx.payload;
    j["signature"] = tx.signature;

    auto body = j.dump();

    for (auto p : peers.getPeers())
        sendToPeer(p.address, "/gossip/tx", body);
}

void GossipProtocol::gossipBlock(const Block& block) {
    json j = block.serialize();  // dein serialize() ist JSON-fähig?
    auto body = j.dump();

    for (auto p : peers.getPeers())
        sendToPeer(p.address, "/gossip/block", body);
}

