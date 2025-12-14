#pragma once
#include <string>
#include <vector>
#include <ctime>


struct RunMetrics {
    std::string filename;

    double rto_ms = 0.0;
    double snapshot_ms = 0.0;
    double restore_ms = 0.0;
    double verify_ms = 0.0;

    bool passed = false;
};



class RtoRpoAnalyzer {
public:
    RtoRpoAnalyzer(const std::string& reportsDir);

    std::vector<RunMetrics> analyzeAll();
    bool parseFile(const std::string& path, RunMetrics& out);
    static bool parseCtime(const std::string& s, std::time_t& out);
    static double diffMs(std::time_t a, std::time_t b);

    bool writeSummaryJson(const std::string& outPath, const std::vector<RunMetrics>& runs) const;
    bool writeCsv(const std::string& outPath, const std::vector<RunMetrics>& runs) const;

private:
    std::string dir;
};

