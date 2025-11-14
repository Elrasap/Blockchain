#pragma once
#include <string>
#include <chrono>

struct Incident {
    std::string type;         // "NODE_FAIL", "SECURITY", "PERFORMANCE"
    std::string message;
    std::string severity;     // "INFO","WARN","CRITICAL"
    std::chrono::system_clock::time_point timestamp;
};

