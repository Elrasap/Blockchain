#pragma once
#include <string>
#include <vector>
#include "analytics/rtoRpoAnalyzer.hpp"
#include "analytics/forecaster.hpp"

struct ReliabilityStatus {
    bool integrityOk;
    bool perfOk;
    bool chaosOk;
    bool forecastOk;
    double avgRto;
    double passRate;
    int anomalies;
};

class ReliabilityGuard {
public:
    ReliabilityGuard(const std::string& reportsDir, const std::string& binaryPath);
    ReliabilityStatus evaluate(double maxAvgRto, double minPassRate, int maxAnomalies);
    bool writeJson(const std::string& path, const ReliabilityStatus& s);
    void printStatus(const ReliabilityStatus& s);
private:
    std::string dir;
    std::string bin;
    bool verifyBinaryChecksum();
};

