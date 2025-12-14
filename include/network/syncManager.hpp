#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <thread>
#include <atomic>

class Blockchain;
class PeerManager;

class SyncManager {
public:
    SyncManager(Blockchain& chain, PeerManager& peers);
    ~SyncManager();

    void start();
    void stop();


    void handleInv(const std::array<uint8_t,32>& hash);
    void handleGetBlock(const std::array<uint8_t,32>& hash, int fd);
    void handleBlock(const class Block& block);

private:
    void loop();
    bool hasBlock(const std::array<uint8_t,32>& hash) const;
    void requestBlock(const std::array<uint8_t,32>& hash);

private:
    Blockchain& chain_;
    PeerManager& peers_;

    std::thread worker_;
    std::atomic<bool> running{false};


    std::mutex mtx_;
    std::array<uint8_t,32> wanted_{};
    bool haveWanted_ = false;
};


extern SyncManager* global_sync;

