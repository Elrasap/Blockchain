#include "obs/metrics.hpp"
#include <sstream>

Metrics& Metrics::instance() {
    static Metrics m;
    return m;
}

void Metrics::incCounter(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mtx);
    counters[name] += value;
}

void Metrics::setGauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mtx);
    gauges[name] = value;
}

void Metrics::observe(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mtx);
    if (hist.find(name) == hist.end()) hist[name] = value;
    else hist[name] = (hist[name] + value) / 2.0;
}

std::string Metrics::renderPrometheus() {
    std::lock_guard<std::mutex> lock(mtx);
    std::ostringstream out;
    for (const auto& kv : counters) out << kv.first << " " << kv.second << "\n";
    for (const auto& kv : gauges) out << kv.first << " " << kv.second << "\n";
    for (const auto& kv : hist) out << kv.first << " " << kv.second << "\n";
    return out.str();
}

void Metrics::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    counters.clear();
    gauges.clear();
    hist.clear();
}

