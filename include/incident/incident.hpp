#pragma once
#include <string>
#include <chrono>

struct Incident {
    std::string type;
    std::string message;
    std::string severity;
    std::chrono::system_clock::time_point timestamp;
};

