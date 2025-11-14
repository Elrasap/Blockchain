#include "analytics/trendReporter.hpp"
#include <fstream>
#include <algorithm>
#include <iomanip>

bool TrendReporter::writeTrendCsv(const std::string& path, const std::vector<RunMetrics>& runs) {
    std::ofstream out(path);
    if (!out) return false;
    out << "index,file,rto_ms,restore_ms,snapshot_ms,verify_ms,passed\n";
    int idx = 0;
    for (const auto &r : runs) {
        out << idx++ << "," << r.filename << ","
            << std::fixed << std::setprecision(0) << r.rto_ms << ","
            << r.restore_ms << "," << r.snapshot_ms << "," << r.verify_ms << "," << (r.passed ? "1" : "0") << "\n";
    }
    return true;
}

