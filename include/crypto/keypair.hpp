#pragma once
#include <array>
#include <cstdint>

struct KeyPair {
    std::array<uint8_t, 32> publicKey;
    std::array<uint8_t, 32> privateKey;

    static KeyPair generate();
    static KeyPair fromSeed(const std::array<uint8_t, 32>& seed);
};

