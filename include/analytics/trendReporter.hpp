#pragma once
#include <string>
#include <vector>
#include "analytics/rto_rpo_analyzer.hpp"

class TrendReporter {
public:
    static bool writeTrendCsv(const std::string& path, const std::vector<RunMetrics>& runs);
};

