#include "network/syncProtocol.hpp"
#include "network/peerManager.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

SyncProtocol::SyncProtocol(PeerManager& p)
    : peers(p) {}

void SyncProtocol::requestPeerHeights() {
    for (auto peer : peers.getPeers()) {
        httplib::Client cli(peer.address.c_str());
        auto res = cli.Get("/chain/height");

        if (res && res->status == 200) {
            uint64_t height = std::stoull(res->body);
            peers.updatePeerHeight(peer.address, height);
        }
    }
}

void SyncProtocol::syncMissingBlocks(uint64_t localHeight) {
    for (auto peer : peers.getPeers()) {
        if (peer.height <= localHeight) continue;

        for (uint64_t h = localHeight + 1; h <= peer.height; ++h) {
            httplib::Client cli(peer.address.c_str());
            auto res = cli.Get(("/chain/block/" + std::to_string(h)).c_str());

            if (res && res->status == 200) {
                try {
                    json j = json::parse(res->body);
                    Block b = Block::deserialize(j.dump());
                    // TODO: your chain.appendBlock(b)
                    std::cout << "[SYNC] Received block " << h
                              << " from " << peer.address << "\n";
                }
                catch (...) {
                    std::cerr << "[SYNC] Failed to parse block\n";
                }
            }
        }
    }
}

