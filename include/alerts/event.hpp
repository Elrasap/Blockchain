#pragma once
#include <string>
#include <chrono>

struct Event {
    std::string type;
    std::string message;
    double value;
    std::chrono::system_clock::time_point timestamp;
};

