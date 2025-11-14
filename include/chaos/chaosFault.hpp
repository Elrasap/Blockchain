#pragma once
#include <string>
#include <chrono>

enum class FaultType { Crash, LatencySpike, PacketLoss, CpuSpike, DiskError };

struct ChaosFault {
    FaultType type;
    int targetNode;                 // -1 = cluster-wide
    std::chrono::milliseconds duration{0};
    double intensity{0.0};          // z.B. Latenz in ms, Loss in %, CPU %
    std::string note;               // Kontext
};

