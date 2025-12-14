#pragma once
#include <string>
#include <chrono>

class Tracer {
public:
    explicit Tracer(const std::string& name);
    ~Tracer();
private:
    std::string name;
    std::chrono::steady_clock::time_point start;
};

