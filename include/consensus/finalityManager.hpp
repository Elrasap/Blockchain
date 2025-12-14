#pragma once
#include <vector>
#include <map>
#include <array>
#include <cstdint>
#include <mutex>
#include "core/block.hpp"
#include "storage/blockStore.hpp"
#include "core/validation.hpp"

struct FinalityState {
    std::array<uint8_t,32> hash;
    uint64_t height;
    bool finalized;
};

class FinalityManager {
public:
    explicit FinalityManager(BlockStore* store);
    void markCommitted(const std::array<uint8_t,32>& hash, uint64_t height);
    void finalize(uint64_t height);
    bool isFinalized(uint64_t height) const;
    bool detectFork(const Block& newBlock);
    FinalityState latest() const;
    void clear();

private:
    BlockStore* store;
    std::map<uint64_t, FinalityState> states;
    mutable std::mutex mtx;
};

