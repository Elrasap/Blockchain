#pragma once
#include <string>
#include <vector>

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
    explicit RtoRpoAnalyzer(const std::string& reportsDir);
    std::vector<RunMetrics> analyzeAll();
    bool writeSummaryJson(const std::string& outPath, const std::vector<RunMetrics>& runs) const;
    bool writeCsv(const std::string& outPath, const std::vector<RunMetrics>& runs) const;
private:
    std::string dir;
    bool parseFile(const std::string& path, RunMetrics& out);
    static bool parseCtime(const std::string& s, std::time_t& out);
    static double diffMs(std::time_t a, std::time_t b);
};

