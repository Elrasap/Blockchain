#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "crypto/hash.hpp"
#include "crypto/types.hpp"
#include "block.hpp"

class BlockBuilder {
public:
    void collectTransactions(uint64_t limit);
    void assembleBlock();
    void sealBlock();
};

