#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "core/block.hpp"

class Blockchain;
class Mempool;
class PeerManager;

class SyncManager {
public:
    SyncManager(Blockchain& chain, PeerManager& peers, Mempool* mempool = nullptr);
    ~SyncManager();

    void start();
    void stop();
    void sendTip(int peerFd);

    void handleInv(const std::array<uint8_t, 32>& hash);
    void handleGetBlock(const std::array<uint8_t, 32>& hash, int fd);
    bool handleBlock(const Block& block);

private:
    void loop();
    bool hasBlock(const std::array<uint8_t, 32>& hash) const;
    void requestBlock(const std::array<uint8_t, 32>& hash);
    void announce(const Block& block);

    Blockchain& chain_;
    PeerManager& peers_;
    Mempool* mempool_;

    std::thread worker_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
    std::vector<std::array<uint8_t, 32>> wanted_;
    std::map<uint64_t, Block> pending_;
};
