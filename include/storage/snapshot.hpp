#pragma once
#include <string>
#include <vector>
#include <array>

#include "core/block.hpp"
#include "core/state.hpp"

class SnapshotManager {
public:
    void createSnapshot(const State& state, uint64_t height);
    void restoreSnapshot(const std::string& filePath);
    std::vector<std::string> listSnapshots() const;
    void createFromBlock(const Block& block, uint64_t intervalBlocks);
    void applySnapshotToState(State& state);
    void transferSnapshot(uint64_t peer);
};

