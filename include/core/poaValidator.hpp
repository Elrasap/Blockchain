#pragma once
#include <vector>
#include <cstdint>
#include "core/block.hpp"

// Using libsodium
bool signBlockHeader(BlockHeader& header,
                     const std::vector<uint8_t>& privKey,
                     const std::vector<uint8_t>& pubKey);

bool verifyBlockHeaderSignature(const BlockHeader& header);

