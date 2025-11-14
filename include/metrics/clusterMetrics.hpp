#pragma once
#include <vector>
#include <map>
#include "metrics/metricsCollector.hpp"

class ClusterMetrics {
public:
    ClusterMetrics(int clusterSize);
    void simulateUpdate();
    std::map<std::string,double> aggregate() const;
private:
    std::vector<MetricsCollector> collectors;
};

