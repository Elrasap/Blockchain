#pragma once
#include <cstdint>
#include <unordered_map>
#include <chrono>

class LivenessTracker {
public:
    void txSubmitted(uint64_t txId);
    void txCommitted(uint64_t txId);
    void latency(uint64_t txId);

private:
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> submitted_;
};

