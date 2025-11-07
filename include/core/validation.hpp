#pragma once
#include "core/transaction.hpp"
#include "core/block.hpp"

namespace Validation {

bool validateTransaction(const Transaction& tx);
bool validateBlock(const Block& block, const Block& prev);

}

