#pragma once
#include <string>
#include <chrono>

enum class FaultType { Crash, LatencySpike, PacketLoss, CpuSpike, DiskError };

struct ChaosFault {
    FaultType type;
    int targetNode;
    std::chrono::milliseconds duration{0};
    double intensity{0.0};
    std::string note;
};

