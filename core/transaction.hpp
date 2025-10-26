#pragma once

#include <vector>
#include <string>
#include "crypto/hash.hpp"
#include "crypto/types.hpp"
#include "crypto/signature.hpp"

using namespace std;

class Transaction {
public:
    B32 senderPubkey;
    B64 nonce;
    B32 payload;
    B64 signature;
    B64 fee;

    vector<uint8_t> serializeWithoutSignature() const;
    vector<uint8_t> serialize() const;
    void deserialize(const vector<uint8_t>& data);
    B32 hash(IHashAlgorithm& hasher) const;
    B32 getHash() const;
    uint64_t timestampImpact() const;
    string toString() const;
    bool sign(const KeyPair& keyPair, const IHashAlgorithm& hash);
    bool verifySignature(const IHashAlgorithm& hash) const;
    B32 getSenderAddress() const;
};
