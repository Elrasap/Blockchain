#include "network/syncProtocol.hpp"
#include "network/peerManager.hpp"
#include "core/block.hpp"
#include <thirdparty/httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

SyncProtocol::SyncProtocol(PeerManager& p)
    : peers(p) {}

// ----------------------------------------------------
// Anfrage der Block-Höhen der Peers
// ----------------------------------------------------
void SyncProtocol::requestPeerHeights() {
    for (auto peer : peers.getPeers()) {

        httplib::Client cli(peer.address.c_str());
        auto res = cli.Get("/chain/height");

        if (!res || res->status != 200)
            continue;

        try {
            uint64_t height = std::stoull(res->body);
            peers.updatePeerHeight(peer.address, height);
        }
        catch (...) {
            std::cerr << "[SYNC] Failed to parse peer height from "
                      << peer.address << "\n";
        }
    }
}

// ----------------------------------------------------
// Fehlenede Blocks holen
// ----------------------------------------------------
void SyncProtocol::syncMissingBlocks(uint64_t localHeight) {

    for (auto peer : peers.getPeers()) {
        if (peer.height <= localHeight)
            continue;

        for (uint64_t h = localHeight + 1; h <= peer.height; ++h) {

            httplib::Client cli(peer.address.c_str());
            auto res = cli.Get(("/chain/block/" + std::to_string(h)).c_str());

            if (!res || res->status != 200)
                continue;

            try {
                // JSON → Block-Daten
                json j = json::parse(res->body);

                // JSON → string
                std::string raw = j.dump();

                // string → vector<uint8_t>
                std::vector<uint8_t> buf(raw.begin(), raw.end());

                // Deserialisieren
                Block b = Block::deserialize(buf);

                // TODO: append to chain
                std::cout << "[SYNC] Received block " << h
                          << " from " << peer.address << "\n";
            }
            catch (...) {
                std::cerr << "[SYNC] Failed to parse or deserialize block from "
                          << peer.address << "\n";
            }
        }
    }
}

