#pragma once
#include <string>
#include <vector>
#include <ctime>              // <-- ADD THIS

struct RunMetrics {
    std::string filename;
    bool passed;
    double rto_ms;              // time from ClusterCrash.start -> Restore.end
    double snapshot_ms;
    double restore_ms;
    double verify_ms;
};

class RtoRpoAnalyzer {
public:
    RtoRpoAnalyzer(const std::string& reportsDir);

    std::vector<RunMetrics> analyzeAll();
    bool parseFile(const std::string& path, RunMetrics& out);   // <-- ADD THIS
    static bool parseCtime(const std::string& s, std::time_t& out);
    static double diffMs(std::time_t a, std::time_t b);

    bool writeSummaryJson(const std::string& outPath, const std::vector<RunMetrics>& runs) const;
    bool writeCsv(const std::string& outPath, const std::vector<RunMetrics>& runs) const;

private:
    std::string dir;
};

