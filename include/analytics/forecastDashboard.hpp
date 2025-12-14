#pragma once
#include <string>
#include <vector>
#include "analytics/rtoRpoAnalyzer.hpp"
#include "analytics/forecaster.hpp"

struct ForecastSummary {
    FitResult fit;
    std::vector<double> forecast;
    std::vector<int> anomalies;
    int n;
};

class ForecastDashboard {
public:
    explicit ForecastDashboard(const std::string& reportsDir);
    ForecastSummary run(int horizon, double zThresh, double pctThresh);
    bool writeJson(const std::string& path, const ForecastSummary& s, const std::vector<double>& y);
    bool writeCsv(const std::string& path, const ForecastSummary& s, const std::vector<double>& y);
private:
    std::string dir;
};

