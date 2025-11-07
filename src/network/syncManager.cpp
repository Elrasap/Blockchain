#include "network/syncManager.hpp"
#include "core/validation.hpp"
#include "network/messages.hpp"
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <iostream>

using namespace std;

SyncManager::SyncManager(BlockStore* s) : store(s) {}

void SyncManager::attachPeer(int peer_fd) {
    lock_guard<mutex> lock(mtx);
    peer_fds.push_back(peer_fd);
}

vector<int> SyncManager::getPeers() {
    lock_guard<mutex> lock(mtx);
    return peer_fds;
}

void SyncManager::handleInv(const array<uint8_t, 32>& hash) {
    lock_guard<mutex> lock(mtx);
    if (known_blocks.count(hash)) return;
    known_blocks[hash] = true;
    Message msg;
    msg.type = MessageType::GETBLOCK;
    msg.payload.assign(hash.begin(), hash.end());
    auto encoded = encodeMessage(msg);
    for (auto fd : peer_fds) send(fd, encoded.data(), encoded.size(), 0);
}

void SyncManager::handleGetBlock(const array<uint8_t, 32>& hash, int peer_fd) {
    auto blocks = store->loadAllBlocks();
    for (auto& b : blocks) {
        auto h = b.hash();
        if (memcmp(h.data(), hash.data(), 32) == 0) {
            Message msg;
            msg.type = MessageType::BLOCK;
            auto bytes = b.serialize();
            msg.payload = bytes;
            auto encoded = encodeMessage(msg);
            send(peer_fd, encoded.data(), encoded.size(), 0);
            break;
        }
    }
}

void SyncManager::handleBlock(const Block& block) {
    auto blocks = store->loadAllBlocks();
    if (!blocks.empty()) {
        Block prev = blocks.back();
        if (!Validation::validateBlock(block, prev)) return;
    }
    store->appendBlock(block);
    auto h = block.hash();
    known_blocks[h] = true;
    announceBlock(block);
}

void SyncManager::announceBlock(const Block& block) {
    auto h = block.hash();
    Message msg;
    msg.type = MessageType::INV;
    msg.payload.assign(h.begin(), h.end());
    auto encoded = encodeMessage(msg);
    lock_guard<mutex> lock(mtx);
    for (auto fd : peer_fds) send(fd, encoded.data(), encoded.size(), 0);
}

