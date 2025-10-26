#pragma once
#include <vector>
#include <string>
#include "crypto/types.hpp"
#include "crypto/hash.hpp"
#include "block.hpp"

using namespace std;

namespace validation {
    bool validateBlock(const BlockHeader& header);
    bool validateTransaction(const Transaction& transaction);
    bool validateChain(const Block& block);
}

bool validateTransacton(const Transaction& tx) {

