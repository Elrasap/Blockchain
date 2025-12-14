#include "network/fastSyncManager.hpp"
#include "network/messages.hpp"
#include "core/crypto.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

static std::array<uint8_t,32> concatHash(const std::array<uint8_t,32>& a, const std::array<uint8_t,32>& b) {
    std::vector<uint8_t> buf;
    buf.reserve(64);
    buf.insert(buf.end(), a.begin(), a.end());
    buf.insert(buf.end(), b.begin(), b.end());
    return crypto::sha256(buf);
}

static MerkleProof buildProofFromLeaves(const std::vector<std::array<uint8_t,32>>& leaves, size_t index) {
    MerkleProof p;
    p.leaf = leaves[index];
    std::vector<std::array<uint8_t,32>> level = leaves;
    std::vector<size_t> idxs;
    idxs.push_back(index);
    while (level.size() > 1) {
        std::vector<std::array<uint8_t,32>> next;
        for (size_t i = 0; i < level.size(); i += 2) {
            std::array<uint8_t,32> left = level[i];
            std::array<uint8_t,32> right = (i + 1 < level.size()) ? level[i+1] : level[i];
            next.push_back(concatHash(left, right));
        }
        size_t cur = idxs.back();
        size_t sib = (cur % 2 == 0) ? (cur + 1 < level.size() ? cur + 1 : cur) : cur - 1;
        p.path.push_back(level[sib]);
        p.left.push_back(cur % 2 != 0);
        idxs.push_back(cur / 2);
        level.swap(next);
    }
    p.root = level[0];
    return p;
}

FastSyncManager::FastSyncManager(BlockStore* s, LightClient* c) : store(s), client(c) {}

void FastSyncManager::handleGetHeader(int peer_fd, uint64_t height) {
    std::lock_guard<std::mutex> lock(mtx);
    auto blocks = store->loadAllBlocks();
    BlockHeader target{};
    bool found = false;
    for (const auto& b : blocks) {
        if (b.header.height == height) { target = b.header; found = true; break; }
    }
    if (!found) return;
    auto payload = encodeHeader(target);
    Message m{MessageType::HEADER, payload};
    auto bytes = encodeMessage(m);
    send(peer_fd, bytes.data(), bytes.size(), 0);
}

void FastSyncManager::handleHeader(const BlockHeader& h) {
    std::lock_guard<std::mutex> lock(mtx);
    client->ingestHeader(h);
}

void FastSyncManager::handleGetProofTx(int peer_fd, const std::array<uint8_t,32>& txHash) {
    std::lock_guard<std::mutex> lock(mtx);
    auto blocks = store->loadAllBlocks();
    for (const auto& b : blocks) {
        std::vector<std::array<uint8_t,32>> leaves;
        leaves.reserve(b.transactions.size());
        for (const auto& tx : b.transactions) leaves.push_back(tx.hash());
        for (size_t i = 0; i < leaves.size(); ++i) {
            if (leaves[i] == txHash) {
                MerkleProof proof = buildProofFromLeaves(leaves, i);
                auto payload = encodeMerkleProof(proof);
                Message m{MessageType::PROOF_TX, payload};
                auto bytes = encodeMessage(m);
                send(peer_fd, bytes.data(), bytes.size(), 0);
                return;
            }
        }
    }
}

void FastSyncManager::handleProofTx(const MerkleProof& proof) {
    std::lock_guard<std::mutex> lock(mtx);
    client->verifyTxInBlock(proof, proof.root);
}

void FastSyncManager::broadcastHeader(const BlockHeader& h) {
    std::lock_guard<std::mutex> lock(mtx);
    auto payload = encodeHeader(h);
    Message m{MessageType::HEADER, payload};
    auto bytes = encodeMessage(m);
    for (int fd : peers) send(fd, bytes.data(), bytes.size(), 0);
}

void FastSyncManager::attachPeer(int peer_fd) {
    std::lock_guard<std::mutex> lock(mtx);
    if (std::find(peers.begin(), peers.end(), peer_fd) == peers.end()) peers.push_back(peer_fd);
}

void FastSyncManager::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    peers.clear();
}

