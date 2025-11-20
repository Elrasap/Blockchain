#include "network/syncManager.hpp"

#include "core/block.hpp"
#include "core/blockchain.hpp"
#include "network/peerManager.hpp"
#include "network/messages.hpp"

#include <sys/socket.h>
#include <unistd.h>   // ::send
#include <iostream>
#include <cstring>

SyncManager* global_sync = nullptr;

SyncManager::SyncManager(Blockchain& chain, PeerManager& peers)
    : chain_(chain), peers_(peers)
{}

SyncManager::~SyncManager() {
    stop();
}

void SyncManager::start() {
    running = true;
    worker_ = std::thread(&SyncManager::loop, this);
}

void SyncManager::stop() {
    running = false;
    if (worker_.joinable())
        worker_.join();
}

bool SyncManager::hasBlock(const std::array<uint8_t,32>& hash) const {
    auto& ch = chain_.getChain();
    for (auto& b : ch) {
        if (b.hash() == hash)
            return true;
    }
    return false;
}

void SyncManager::requestBlock(const std::array<uint8_t,32>& hash) {
    Message msg;
    msg.type = MessageType::GETBLOCK;
    msg.payload.assign(hash.begin(), hash.end());
    auto encoded = encodeMessage(msg);

    // über alle Peers rausschicken
    peers_.broadcastRaw(encoded);
}

void SyncManager::handleInv(const std::array<uint8_t,32>& hash) {
    if (hasBlock(hash))
        return; // schon vorhanden — ignorieren

    {
        std::lock_guard<std::mutex> lock(mtx_);
        wanted_ = hash;
        haveWanted_ = true;
    }
}

void SyncManager::handleGetBlock(const std::array<uint8_t,32>& hash, int fd) {
    auto& ch = chain_.getChain();
    for (auto& b : ch) {
        if (b.hash() == hash) {
            auto bytes = b.serialize();

            Message msg;
            msg.type = MessageType::BLOCK;
            msg.payload = bytes;

            auto out = encodeMessage(msg);
            ::send(fd, out.data(), out.size(), 0);
            return;
        }
    }
}

void SyncManager::handleBlock(const Block& block) {
    if (!chain_.appendBlock(block)) {
        std::cerr << "[Sync] incoming BLOCK rejected\n";
        return;
    }

    std::cout << "[Sync] accepted block height=" << block.header.height << "\n";
}

void SyncManager::loop() {
    while (running) {
        std::array<uint8_t,32> want{};
        bool wantSet = false;

        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (haveWanted_) {
                want = wanted_;
                wantSet = true;
                haveWanted_ = false;
            }
        }

        if (wantSet) {
            std::cout << "[Sync] requesting block...\n";
            requestBlock(want);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

