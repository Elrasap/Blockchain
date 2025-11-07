#include "consensus/finalityManager.hpp"

FinalityManager::FinalityManager(BlockStore* s) : store(s) {}

void FinalityManager::markCommitted(const std::array<uint8_t,32>& hash, uint64_t height) {
    std::lock_guard<std::mutex> lock(mtx);
    FinalityState st;
    st.hash = hash;
    st.height = height;
    st.finalized = false;
    states[height] = st;
}

void FinalityManager::finalize(uint64_t height) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = states.find(height);
    if (it != states.end()) it->second.finalized = true;
}

bool FinalityManager::isFinalized(uint64_t height) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = states.find(height);
    if (it == states.end()) return false;
    return it->second.finalized;
}

bool FinalityManager::detectFork(const Block& newBlock) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = states.find(newBlock.header.height);
    if (it == states.end()) return false;
    const FinalityState& known = it->second;
    if (known.hash != newBlock.hash()) return true;
    return false;
}

FinalityState FinalityManager::latest() const {
    std::lock_guard<std::mutex> lock(mtx);
    if (states.empty()) return FinalityState{};
    auto it = states.rbegin();
    return it->second;
}

void FinalityManager::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    states.clear();
}

