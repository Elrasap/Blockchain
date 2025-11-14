#pragma once
#include <string>
#include <vector>
#include "ops/reliabilityGuard.hpp"
#include "analytics/rtoRpoAnalyzer.hpp"

struct RtoRecord {
    std::string filename;
    double rto_ms;
    double restore_ms;
    bool passed;
};

class HistoryStore {
public:
    explicit HistoryStore(const std::string& dbPath);
    bool init();
    bool insertReliability(const ReliabilityStatus& s);
    bool insertRtoRecords(const std::vector<RunMetrics>& runs);
    std::vector<RtoRecord> loadRecentRto(int limit = 20);
private:
    std::string path;
    bool exec(const std::string& sql);
};

