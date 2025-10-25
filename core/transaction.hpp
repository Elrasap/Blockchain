#pragma once

#include <vector>
#include <string>
#include "crypto/hash.hpp"
#include "crypto/types.hpp"

class Transaction {
public:
    B32 senderPubkey;
    B64 nonce;
    B32 payload;
    B64 signature;
    B64 fee;
    std::vector<uint8_t> serializeWithoutSignature() const;
    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& data);

    B32 hash(IHashAlgorithm& hasher) const;
    B32 getHash() const;
    bool verifySignature() const;

    uint64_t estimateFeeImpact() const;
    std::string toString() const;
};

