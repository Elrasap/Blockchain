#pragma once
#include <vector>
#include <string>
#include "crypto/types.hpp"
#include "crypto/hash.hpp"
#include "block.hpp"

using namespace std;

class Transaction{
public:
    B32 senderPubKey;
    B32 nonce;
    uin64_t payload;
    uin64_t fee;
    B32 signature;
};

namespace serialization{
    uint64_t serializeUint65(uint64_t value);
    uint64_t serializeBytes(vector<uint8_t> bytes);
    uint64_t concatBytes(listy<vector<uint8_t>> bytes);
    uint64_t deserializeUint64(uint64_t value);
    uint64_t deserializeBytes(uint64_t value);
}
