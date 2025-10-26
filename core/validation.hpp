#pragma once
#include <vector>
#include <string>
#include "crypto/types.hpp"
#include "crypto/hash.hpp"
#include "block.hpp"

using namespace std;

namespace validation {
    bool validateBlock(const BlockHeader& header);
    bool validateTransaction(const Transaction& tx);
    bool validateChain(const Block& block);
}


