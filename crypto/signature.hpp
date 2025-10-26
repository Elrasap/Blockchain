#pragma once
#include <string>
#include <vector>
#include "crypto/types.hpp"

B64 sign(const std::vector<uint8_t>& messageBytes, const std::array<uint8_t, 32>& privateKey);
bool verify(const std::vector<uint8_t>& messageBytes, const B32& publicKey, const B64& signature);
B64 signatureSize();
B32 publicKeySize();

