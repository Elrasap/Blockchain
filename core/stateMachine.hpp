#pragma once
#include <string>
#include "types.hpp"

class StateMachine {
public:
    void applyBlock(const Block& block);
    Hash32 getStateRoot() const;
    void snapshot();
};

