#pragma once
#include <vector>
#include <array>
#include <string>
#include <cstdint>

namespace crypto {

struct KeyPair {
    std::vector<uint8_t> publicKey;   // 32 bytes
    std::vector<uint8_t> privateKey;  // 64 bytes
};

std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data);
std::string toHex(const std::array<uint8_t, 32>& hash);

KeyPair generateKeyPair();
std::vector<uint8_t> sign(const std::vector<uint8_t>& msg,
                          const std::vector<uint8_t>& priv);

bool verify(const std::vector<uint8_t>& msg,
            const std::vector<uint8_t>& sig,
            const std::vector<uint8_t>& pub);

} // namespace crypto

