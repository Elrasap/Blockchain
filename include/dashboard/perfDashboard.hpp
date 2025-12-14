#pragma once
#include <string>
#include <vector>
#include "analytics/rtoRpoAnalyzer.hpp"

struct PerfSummary {
    double avgRto;
    double p95Rto;
    double avgRestore;
    double chaosPassRate;
    int totalRuns;
    int regressions;
};

class PerfDashboard {
public:
    explicit PerfDashboard(const std::string& reportsDir);
    PerfSummary compute();
    bool writeJson(const std::string& path, const PerfSummary& s);
    bool writeCsv(const std::string& path, const PerfSummary& s);
    void printAsciiChart(const std::vector<RunMetrics>& runs);
private:
    std::string dir;
};

