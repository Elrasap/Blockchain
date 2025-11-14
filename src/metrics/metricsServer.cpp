#include "metrics/metrics_server.hpp"
#include <iostream>
#include <sstream>

MetricsServer::MetricsServer(int p) : port(p) {}

void MetricsServer::serve(const std::map<std::string,double>& metrics) {
    std::ostringstream ss;
    ss << "---- Cluster Metrics ----\n";
    for (auto& kv : metrics)
        ss << kv.first << ": " << kv.second << "\n";
    ss << "-------------------------\n";
    std::cout << ss.str();
}

