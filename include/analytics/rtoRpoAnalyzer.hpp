#pragma once
#include <string>
#include <vector>
#include <ctime>              // <-- ADD THIS


struct RunMetrics {
    std::string filename;

    double rto_ms = 0.0;        // Main Recovery Time
    double snapshot_ms = 0.0;   // Snapshot phase duration
    double restore_ms = 0.0;    // Restore phase duration
    double verify_ms = 0.0;     // Verification phase duration

    bool passed = false;        // Whether the run passed checks
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

