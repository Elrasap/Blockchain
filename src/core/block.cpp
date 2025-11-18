#include "core/block.hpp"
#include "core/crypto.hpp"
#include <cstring>
#include <stdexcept>

using namespace std;

namespace {

// little-endian uint64 schreiben
void writeU64(vector<uint8_t>& buf, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
    }
}

// little-endian uint64 lesen
bool readU64(const vector<uint8_t>& data, size_t& offset, uint64_t& out) {
    if (offset + 8 > data.size()) return false;
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;
    out = v;
    return true;
}

// Länge + Bytes schreiben
void writeVarBytes(vector<uint8_t>& buf, const vector<uint8_t>& bytes) {
    uint64_t len = bytes.size();
    writeU64(buf, len);
    buf.insert(buf.end(), bytes.begin(), bytes.end());
}

// Länge + Bytes lesen
bool readVarBytes(const vector<uint8_t>& data, size_t& offset, vector<uint8_t>& out) {
    uint64_t len = 0;
    if (!readU64(data, offset, len)) return false;
    if (offset + len > data.size()) return false;

    out.assign(data.begin() + static_cast<long>(offset),
               data.begin() + static_cast<long>(offset + len));
    offset += static_cast<size_t>(len);
    return true;
}

// Header serialisieren; je nach Flag mit/ohne signature.
vector<uint8_t> serializeHeaderInternal(const BlockHeader& h, bool includeSignature) {
    vector<uint8_t> buf;
    // height + timestamp
    writeU64(buf, h.height);
    writeU64(buf, h.timestamp);

    // prevHash + merkleRoot
    buf.insert(buf.end(), h.prevHash.begin(), h.prevHash.end());
    buf.insert(buf.end(), h.merkleRoot.begin(), h.merkleRoot.end());

    // validatorPubKey (mit Längenfeld)
    writeVarBytes(buf, h.validatorPubKey);

    if (includeSignature) {
        writeVarBytes(buf, h.signature);
    }

    return buf;
}

} // namespace


// === Block-MerkleRoot ===

std::array<uint8_t, 32> Block::calculateMerkleRoot() const {
    std::vector<std::array<uint8_t, 32>> layer;

    // ====== Edge Case: Keine Transaktionen ======
    if (transactions.empty()) {
        // Leeres MerkleRoot = SHA256("")
        return crypto::sha256(std::vector<uint8_t>{});
    }

    // ====== Erste Ebene: Hash jeder Transaktion ======
    layer.reserve(transactions.size());
    for (const auto& tx : transactions) {
        layer.push_back(tx.hash());
    }

    // ====== Merkle-Baum iterativ aufbauen ======
    while (layer.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next;

        for (size_t i = 0; i < layer.size(); i += 2) {
            std::array<uint8_t, 32> left = layer[i];
            std::array<uint8_t, 32> right;

            // Wenn ungerade Anzahl → letzten Hash duplizieren (Bitcoin-Regel)
            if (i + 1 < layer.size()) {
                right = layer[i + 1];
            } else {
                right = left;
            }

            // Concatenate L || R
            std::vector<uint8_t> concat;
            concat.reserve(64);
            concat.insert(concat.end(), left.begin(), left.end());
            concat.insert(concat.end(), right.begin(), right.end());

            // Hash(L || R)
            next.push_back(crypto::sha256(concat));
        }

        layer = std::move(next);
    }

    // ====== Ein Element übrig → MerkleRoot ======
    return layer[0];
}


// === Block-Hash ===
// Hash = SHA256( Header inkl. Signatur )
std::array<uint8_t, 32> Block::hash() const {
    return header.hash();
}

// === Serialize / Deserialize ===

vector<uint8_t> Block::serialize() const {
    // Aktuell serialisieren wir nur den Header.
    // Transaktionen können später ergänzt werden (Body-Format).
    return serializeHeaderInternal(header, /*includeSignature=*/true);
}

Block Block::deserialize(const vector<uint8_t>& data) {
    Block b;
    size_t offset = 0;

    // height
    if (!readU64(data, offset, b.header.height)) {
        return Block{};
    }
    // timestamp
    if (!readU64(data, offset, b.header.timestamp)) {
        return Block{};
    }

    // prevHash + merkleRoot (je 32 Bytes)
    if (offset + 32 + 32 > data.size()) {
        return Block{};
    }
    memcpy(b.header.prevHash.data(), data.data() + offset, 32);
    offset += 32;
    memcpy(b.header.merkleRoot.data(), data.data() + offset, 32);
    offset += 32;

    // validatorPubKey
    if (!readVarBytes(data, offset, b.header.validatorPubKey)) {
        return Block{};
    }

    // signature
    if (!readVarBytes(data, offset, b.header.signature)) {
        return Block{};
    }

    return b;
}

// === Signatur-Helper ===

vector<uint8_t> serializeHeaderForSigning(const BlockHeader& header) {
    // Ohne signature-Feld
    return serializeHeaderInternal(header, /*includeSignature=*/false);
}

std::vector<uint8_t> BlockHeader::toBytes() const {
    std::vector<uint8_t> buf;
    buf.reserve(32 + 32 + 8 + 8 + 4 + validatorPubKey.size() + 4 + signature.size());

    buf.insert(buf.end(), prevHash.begin(), prevHash.end());
    buf.insert(buf.end(), merkleRoot.begin(), merkleRoot.end());

    for (int i = 0; i < 8; i++) buf.push_back((height >> (8 * i)) & 0xFF);
    for (int i = 0; i < 8; i++) buf.push_back((timestamp >> (8 * i)) & 0xFF);

    uint32_t pubLen = validatorPubKey.size();
    for (int i = 0; i < 4; i++) buf.push_back((pubLen >> (8 * i)) & 0xFF);
    buf.insert(buf.end(), validatorPubKey.begin(), validatorPubKey.end());

    uint32_t sigLen = signature.size();
    for (int i = 0; i < 4; i++) buf.push_back((sigLen >> (8 * i)) & 0xFF);
    buf.insert(buf.end(), signature.begin(), signature.end());

    return buf;
}

std::array<uint8_t, 32> BlockHeader::hash() const {
    return crypto::sha256(toBytes());
}

