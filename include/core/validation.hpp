#pragma once
#include "core/transaction.hpp"
#include "core/block.hpp"
#include <vector>

class Transaction;

namespace Validation {

bool validateTransaction(const Transaction& tx);

bool validateBlock(const Block& block, const Block& prev);

bool validateBlockPoA(const Block& block,
                      const Block& prev,
                      const std::vector<uint8_t>& expectedValidatorPubKey);

}

