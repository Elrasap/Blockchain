#pragma once
#include <vector>
#include <array>
#include <string>

#include "crypto/types.hpp"
#include "core/block.hpp"

class BlockStore {
public:
    void appendBlock(const Block& block);
    Block getBlockByHeight(uint64_t height) const;
    Block getLatestBlock() const;
    bool verifyChainIntegrity() const;
};
