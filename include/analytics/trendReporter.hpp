#pragma once
#include <string>
#include <vector>
#include "analytics/rtoRpoAnalyzer.hpp"

class TrendReporter {
public:
    static bool writeTrendCsv(const std::string& path, const std::vector<RunMetrics>& runs);
};

