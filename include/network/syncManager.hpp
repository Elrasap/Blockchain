#pragma once
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include "core/block.hpp"
#include "storage/blockStore.hpp"
#include "network/messages.hpp"
#include "core/validation.hpp"

class SyncManager {
public:
    SyncManager(BlockStore* store);
    void handleInv(const std::array<uint8_t, 32>& hash);
    void handleGetBlock(const std::array<uint8_t, 32>& hash, int peer_fd);
    void handleBlock(const Block& block);
    void announceBlock(const Block& block);
    void attachPeer(int peer_fd);
    std::vector<int> getPeers();

private:
    BlockStore* store;
    std::map<std::array<uint8_t, 32>, bool> known_blocks;
    std::mutex mtx;
    std::vector<int> peer_fds;
};

