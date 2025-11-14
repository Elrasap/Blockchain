#pragma once
#include <string>

struct AlertRule {
    std::string metric;
    double threshold;
    std::string severity;   // "INFO","WARN","CRITICAL"
};

