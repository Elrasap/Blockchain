#pragma once
#include <string>
#include "types.hpp"
#include "block.hpp"

class RaftIntegration {
public:
    void onAppendEntries(const Block& block);
    void onCommit(const uint64_t index);
    void requestSnapshot(uint64_t peer);
};

