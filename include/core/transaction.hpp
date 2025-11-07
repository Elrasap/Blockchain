#pragma once
#include <vector>
#include <array>
#include <cstdint>

std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data);

class Transaction {
public:
    std::vector<uint8_t> senderPubkey;
    std::vector<uint8_t> payload;
    std::vector<uint8_t> signature;
    uint64_t nonce = 0;
    uint64_t fee = 0;

    std::vector<uint8_t> serialize() const;
    void sign(const std::vector<uint8_t>& priv);
    bool verifySignature() const;
    std::array<uint8_t, 32> hash() const;
    void deserialize(const std::vector<uint8_t>& data);
};
