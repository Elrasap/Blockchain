#pragma once
#include <string>
#include <chrono>

struct RecoveryStep {
    std::string name;
    std::string action;
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
    bool success;
    std::string note;
};

