#pragma once

#include <string>
#include <vector>

class stressProfile {
public:
    void recordMetric(const std::string& subsystem, double value);
    void summarize();
};
