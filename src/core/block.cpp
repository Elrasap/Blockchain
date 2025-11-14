#include "core/block.hpp"
#include "core/crypto.hpp"
#include <vector>
#include <cstring>

//
// Serialize BlockHeader → vector<uint8_t>
//
static std::vector<uint8_t> serializeHeader(const BlockHeader& h) {
    std::vector<uint8_t> buf;
    buf.reserve(32 + 32 + 8 + 8 + 8);

    buf.insert(buf.end(), h.prevHash.begin(), h.prevHash.end());
    buf.insert(buf.end(), h.merkleRoot.begin(), h.merkleRoot.end());

    for (int i = 0; i < 8; ++i) buf.push_back((h.height    >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) buf.push_back((h.timestamp >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) buf.push_back((h.nonce     >> (i * 8)) & 0xFF);

    return buf;
}

//
// Merkle-Root über alle Transactions
//
std::array<uint8_t, 32> Block::calculateMerkleRoot() const {
    if (transactions.empty()) {
        std::vector<uint8_t> empty{};
        return crypto::sha256(empty);
    }

    std::vector<std::array<uint8_t, 32>> layer;
    layer.reserve(transactions.size());

    // Hash each transaction
    for (const auto& tx : transactions)
        layer.push_back(tx.hash());

    // Reduce tree
    while (layer.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next;
        next.reserve((layer.size() + 1) / 2);

        for (size_t i = 0; i < layer.size(); i += 2) {
            if (i + 1 < layer.size()) {
                std::vector<uint8_t> buf;
                buf.reserve(64);
                buf.insert(buf.end(), layer[i].begin(), layer[i].end());
                buf.insert(buf.end(), layer[i + 1].begin(), layer[i + 1].end());

                next.push_back(crypto::sha256(buf));
            } else {
                next.push_back(layer[i]); // duplicate last if odd count
            }
        }

        layer = std::move(next);
    }

    return layer[0];
}

//
// Hash des gesamten Blocks → SHA256(Header)
//
std::array<uint8_t, 32> Block::hash() const {
    return crypto::sha256(serializeHeader(header));
}

//
// Serialize complete Block
//
std::vector<uint8_t> Block::serialize() const {
    return serializeHeader(header);
}

//
// Deserialize
//
Block Block::deserialize(const std::vector<uint8_t>& data) {
    Block b;
    size_t offset = 0;

    if (data.size() < 32 + 32 + 8 + 8 + 8)
        return b;  // invalid

    std::memcpy(b.header.prevHash.data(),     &data[offset], 32); offset += 32;
    std::memcpy(b.header.merkleRoot.data(),   &data[offset], 32); offset += 32;

    std::memcpy(&b.header.height,    &data[offset], 8); offset += 8;
    std::memcpy(&b.header.timestamp, &data[offset], 8); offset += 8;
    std::memcpy(&b.header.nonce,     &data[offset], 8); offset += 8;

    return b;
}

