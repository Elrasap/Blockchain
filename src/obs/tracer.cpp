#include "obs/tracer.hpp"
#include "obs/metrics.hpp"
#include <chrono>

Tracer::Tracer(const std::string& name) : name(name), start(std::chrono::steady_clock::now()) {}

Tracer::~Tracer() {
    auto end = std::chrono::steady_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
    Metrics::instance().observe(name + "_seconds", duration);
}

