#pragma once
#include <vector>
#include <string>
#include "crypto/types.hpp"
#include "crypto/hash.hpp"
#include "transaction.hpp"

using namespace std;

class BlockHeader {
public:
    B32 prevHash;
    B32 merkelRoot;
    uint64_t height;
    uint64_t timestamp;
    B32 nonceOrTerm;
    std:string hashAlgoId;
};

class Block {
public:
    BlockHeader header;
    uint64_t transactionCount;
    vector<Transaction> transactions;
};
