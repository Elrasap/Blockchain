#pragma once
#include <cstdint>
#include <string>

class FaultInjector {
public:
    void dropMessages(uint64_t type);
    void delay(uint64_t latencyMs);
    void crashNode(uint64_t id);
};

