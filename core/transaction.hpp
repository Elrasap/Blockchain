#pragma once
#include <vector>
#include <string>
#include "crypto/hash.hpp"
#include "crypto/types.hpp"

class Transaction {
public:
    B32 senderPubkey;
    B65 nonce;
    B32 payload;
    B64 signature;
    B64 fee;

    SHA256 getHash() const {
        SHA256Hash hasher;
        auto bytes = serializeWithoutSignature();
        return hasher.hash(bytes);
    }

    std::vector<uint8_t> serializeWithoutSignature() const;

    void serializeSize() const;
    void serialize() const;
    void deserialize();
    bool verifySignature() const;
    void estimateFeeImpact() const;
};


