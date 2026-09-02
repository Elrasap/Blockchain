#pragma once

#include <string>
#include <vector>

#include "core/transaction.hpp"
#include "dnd/dndState.hpp"

class TransactionValidator {
public:
    explicit TransactionValidator(std::vector<uint8_t> dmPublicKey);

    bool validate(const Transaction& tx,
                  const dnd::DndState& state,
                  std::string& error,
                  bool checkTimestamp = true) const;

    bool validateAndApply(const Transaction& tx,
                          dnd::DndState& state,
                          std::string& error,
                          bool checkTimestamp = true) const;

    const std::vector<uint8_t>& dmPublicKey() const { return dmPublicKey_; }

private:
    std::vector<uint8_t> dmPublicKey_;
};
