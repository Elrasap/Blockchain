#pragma once
#include "core/transaction.hpp"
#include "core/block.hpp"
#include <vector>

class Transaction;

namespace Validation {

bool validateTransaction(const Transaction& tx);

// Basis-Blockvalidierung (Height, prevHash, Merkle, Signatur vorhanden & gültig)
bool validateBlock(const Block& block, const Block& prev);

// Proof-of-Authority: zusätzlich prüfen, dass validatorPubKey == erwarteter DM-Key.
bool validateBlockPoA(const Block& block,
                      const Block& prev,
                      const std::vector<uint8_t>& expectedValidatorPubKey);

}

