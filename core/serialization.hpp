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

