#pragma once
#include <array>
#include <vector>
#include <cstdint>

#include "core/transaction.hpp"   // Vollständiger Typ benötigt

// ----------------------
// BlockHeader
// ----------------------
struct BlockHeader {
    std::array<uint8_t, 32> prevHash{};
    std::array<uint8_t, 32> merkleRoot{};
    uint64_t height = 0;
    uint64_t timestamp = 0;

    std::vector<uint8_t> validatorPubKey;
    std::vector<uint8_t> signature;

    std::vector<uint8_t> toBytes() const;
};

// ----------------------
// Block
// ----------------------
class Block {
public:
    BlockHeader header;
    std::vector<Transaction> transactions;

    std::array<uint8_t, 32> calculateMerkleRoot() const;
    std::array<uint8_t, 32> hash() const;

    std::vector<uint8_t> serialize() const;
    static Block deserialize(const std::vector<uint8_t>& data);
};

// ----------------------
// Block-Signing API
// ----------------------

// Signiert einen BlockHeader mit DM-Key
bool signBlockHeader(
    BlockHeader& header,
    const std::vector<uint8_t>& privKey,
    const std::vector<uint8_t>& pubKey
);

// Verifiziert die BlockHeader-Signatur
bool verifyBlockHeaderSignature(
    const BlockHeader& header
);

