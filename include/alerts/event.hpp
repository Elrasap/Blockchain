#pragma once
#include <string>
#include <chrono>

struct Event {
    std::string type;         // e.g. "METRIC", "SECURITY", "SYSTEM"
    std::string message;      // human readable description
    double value;             // numeric metric if relevant
    std::chrono::system_clock::time_point timestamp;
};

