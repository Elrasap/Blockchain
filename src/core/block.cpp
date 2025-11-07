#include "core/block.hpp"
#include "core/crypto.hpp"
#include <vector>

static std::vector<uint8_t> serializeHeader(const BlockHeader& h) {
    std::vector<uint8_t> buf;
    buf.insert(buf.end(), h.prevHash.begin(), h.prevHash.end());
    buf.insert(buf.end(), h.merkleRoot.begin(), h.merkleRoot.end());
    for (int i = 0; i < 8; ++i) buf.push_back((h.height >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) buf.push_back((h.timestamp >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) buf.push_back((h.nonce >> (i * 8)) & 0xFF);
    return buf;
}

std::array<uint8_t, 32> Block::calculateMerkleRoot() const {
    if (transactions.empty()) {
        std::vector<uint8_t> empty;
        return sha256(empty);
    }
    std::vector<std::array<uint8_t, 32>> layer;
    for (auto& tx : transactions) layer.push_back(tx.hash());
    while (layer.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next;
        for (size_t i = 0; i < layer.size(); i += 2) {
            std::array<uint8_t, 32> left = layer[i];
            std::array<uint8_t, 32> right = (i + 1 < layer.size()) ? layer[i + 1] : layer[i];
            std::vector<uint8_t> buf;
            buf.insert(buf.end(), left.begin(), left.end());
            buf.insert(buf.end(), right.begin(), right.end());
            next.push_back(sha256(buf));
        }
        layer.swap(next);
    }
    return layer[0];
}

std::array<uint8_t, 32> Block::hash() const {
    std::vector<uint8_t> bytes = serializeHeader(header);
    return sha256(bytes);
}

std::vector<uint8_t> Block::serialize() const {
    std::vector<uint8_t> buf = serializeHeader(header);
    return buf;
}

#include <cstring>

Block Block::deserialize(const std::vector<uint8_t>& data) {
    Block b;
    size_t offset = 0;
    if (data.size() < 32 + 32 + 8 + 8 + 8) return b;

    std::memcpy(b.header.prevHash.data(), &data[offset], 32); offset += 32;
    std::memcpy(b.header.merkleRoot.data(), &data[offset], 32); offset += 32;

    uint64_t h = 0;
    std::memcpy(&h, &data[offset], 8); offset += 8;
    b.header.height = h;

    uint64_t ts = 0;
    std::memcpy(&ts, &data[offset], 8); offset += 8;
    b.header.timestamp = ts;

    uint64_t n = 0;
    std::memcpy(&n, &data[offset], 8); offset += 8;
    b.header.nonce = n;

    return b;
}

