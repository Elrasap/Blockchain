#pragma once
#include<array>
#include<vector>
#include "crypto/types.hpp"

class Ed25519Signature {
public:
    static B64 sign(const std::vector<uint8_t>& message, const B32& privateKey);
    static bool verify(const std::vector<uint8_t>& message, const B32& publicKey, const B64& signature);
};

