#include "network/peerDiscovery.hpp"
#include "network/peerManager.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <chrono>
#include <sstream>

using json = nlohmann::json;

PeerDiscovery::PeerDiscovery(PeerManager& mgr, const std::string& path)
    : peers_(mgr), path_(path)
{}

void PeerDiscovery::start() {
    running = true;
    worker_ = std::thread(&PeerDiscovery::loop, this);
}

void PeerDiscovery::stop() {
    running = false;
    if (worker_.joinable())
        worker_.join();

    savePeersFile();
}

void PeerDiscovery::loadPeersFile() {
    std::ifstream f(path_);
    if (!f.good()) {
        std::cerr << "[PeerDiscovery] No peers.json found\n";
        return;
    }

    try {
        json j;
        f >> j;

        if (j.contains("peers")) {
            for (auto& p : j["peers"]) {
                std::string addr = p.get<std::string>();

                auto pos = addr.find(':');
                if (pos == std::string::npos)
                    continue;

                std::string host = addr.substr(0, pos);
                int port = std::stoi(addr.substr(pos + 1));

                std::cout << "[PeerDiscovery] Connecting to " << addr << "\n";
                peers_.connectToPeer(host, port);
            }
        }
    } catch (...) {
        std::cerr << "[PeerDiscovery] Failed to parse peers.json\n";
    }
}

void PeerDiscovery::savePeersFile() {
    auto list = peers_.getPeers();

    json j;
    j["peers"] = json::array();

    for (auto& p : list) {
        j["peers"].push_back(p.address);
    }

    std::ofstream f(path_);
    f << j.dump(2);
    std::cout << "[PeerDiscovery] Saved peers.json\n";
}

void PeerDiscovery::loop() {
    loadPeersFile();

    while (running) {
        // reconnect alle 10 Sekunden
        auto list = peers_.getPeers();
        for (auto& p : list) {

            // hostname und port splitten
            auto pos = p.address.find(':');
            if (pos == std::string::npos)
                continue;

            std::string host = p.address.substr(0, pos);
            int port = std::stoi(p.address.substr(pos+1));

            // Falls nicht verbunden → reconnect versuchen
            if (!peers_.isConnected(host, port)) {
                std::cout << "[PeerDiscovery] Reconnecting " << p.address << "\n";
                peers_.connectToPeer(host, port);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

