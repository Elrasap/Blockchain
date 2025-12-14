#pragma once
#include "core/block.hpp"
#include <vector>

std::vector<uint8_t> encodeHeader(const BlockHeader& h);
BlockHeader decodeHeader(const std::vector<uint8_t>& buf);

