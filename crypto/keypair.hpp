#pragma once
#include <string>
#include <vector>
#include <array>
#include "crypto/types.hpp"

class KeyPair {
public:
    std::array<uint8_t, 32> publicKey;
    std::array<uint8_t, 32> privateKey;
};

array<uint8_t, 32> generate();
array<uint8_t, 32> fromSeed(const array<uint8_t, 32>& bytes);
