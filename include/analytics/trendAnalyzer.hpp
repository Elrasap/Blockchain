#pragma once
#include <string>
#include <vector>

struct TrendEntry {
    std::string date;
    double avgRto;
    double passRate;
    int anomalies;
};

struct TrendSummary {
    double meanRto;
    double medianRto;
    double slope;
    int regressions;
};

class TrendAnalyzer {
public:
    explicit TrendAnalyzer(const std::string& dbPath);
    std::vector<TrendEntry> loadDaily();
    TrendSummary computeSummary(const std::vector<TrendEntry>& data);
    bool writeJson(const std::string& path, const std::vector<TrendEntry>& data, const TrendSummary& s);
    bool writeCsv(const std::string& path, const std::vector<TrendEntry>& data);
private:
    std::string path;
};

