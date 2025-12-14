#pragma once
#include <map>
#include <string>
#include <mutex>

class Metrics {
public:
    static Metrics& instance();
    void incCounter(const std::string& name, double value = 1.0);
    void setGauge(const std::string& name, double value);
    void observe(const std::string& name, double value);
    std::string renderPrometheus();
    void clear();
private:
    Metrics() = default;
    std::map<std::string,double> counters;
    std::map<std::string,double> gauges;
    std::map<std::string,double> hist;
    std::mutex mtx;
};

