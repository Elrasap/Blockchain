#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "types.hpp"

class ChaosRunner {
public:
    void startScenario(const std::string& name);
    void injectFault(const std::string& type, uint64_t durationMs);
    void monitorRecovery(uint64_t timeoutMs);
    void reportResults();
};
