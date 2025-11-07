#pragma once
#include <vector>
#include <array>
#include <string>
#include "core/transaction.hpp"

struct BlockHeader {
    std::array<uint8_t, 32> prevHash{};
    std::array<uint8_t, 32> merkleRoot{};
    uint64_t height = 0;
    uint64_t timestamp = 0;
    uint64_t nonce = 0;
};

class Block {
public:
    BlockHeader header;
    std::vector<Transaction> transactions;
    std::array<uint8_t, 32> calculateMerkleRoot() const;
    std::array<uint8_t, 32> hash() const;
    std::vector<uint8_t> serialize() const;
    static Block deserialize(const std::vector<uint8_t>& data);

};

