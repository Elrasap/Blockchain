#pragma once
#include <string>
#include "types.hpp"
#include "block.hpp"
#include "transaction.hpp"
#include "snapshot.hpp"
#include "state.hpp"

class StateMachine {
public:
    void applyBlock(const Block& block);
    std::string getStateRoot() const;
    void beginApply(uint64_t height);
    void applyTransaction(const Transaction& tx);
    void endApply();
    void rollbackTo(const Snapshot& snapshot);
};

