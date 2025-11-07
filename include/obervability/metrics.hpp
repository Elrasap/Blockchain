#pragma once
#include <cstdint>
#include <string>

class Metrics {
public:
    uint64_t blockTimeSeconds = 0;
    uint64_t raftAppendLatencyMs = 0;
    uint64_t peerCount = 0;
    uint64_t stateApplyLatencyMs = 0;
    uint64_t memoryUsageMb = 0;
    uint64_t diskLatencyMs = 0;
};

