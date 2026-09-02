#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <string>

#include "core/transaction.hpp"

struct BlockHeader {
    std::array<uint8_t, 32> prevHash{};
    std::array<uint8_t, 32> merkleRoot{};
    uint64_t height = 0;
    uint64_t timestamp = 0;

    std::vector<uint8_t> validatorPubKey;
    std::vector<uint8_t> signature;

    std::vector<uint8_t> toSigningBytes() const;
    std::vector<uint8_t> toBytes() const;
    std::array<uint8_t, 32> hash() const;
    static bool deserialize(const std::vector<uint8_t>& data,
                            BlockHeader& out,
                            std::string& error);
};

class Block {
public:
    static constexpr std::size_t MAX_TRANSACTION_COUNT = 1024;
    static constexpr std::size_t MAX_BLOCK_SIZE = 2 * 1024 * 1024;

    BlockHeader header;
    std::vector<Transaction> transactions;

    std::array<uint8_t, 32> calculateMerkleRoot() const;
    std::array<uint8_t, 32> hash() const;

    std::vector<uint8_t> serialize() const;
    static Block deserialize(const std::vector<uint8_t>& data);
    static bool deserialize(const std::vector<uint8_t>& data,
                            Block& out,
                            std::string& error);
};

bool signBlockHeader(BlockHeader& header,
                     const std::vector<uint8_t>& privKey,
                     const std::vector<uint8_t>& pubKey);

bool verifyBlockHeaderSignature(const BlockHeader& header);
