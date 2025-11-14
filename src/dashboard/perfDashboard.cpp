#include "dashboard/perfDashboard.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

PerfDashboard::PerfDashboard(const std::string& reportsDir) : dir(reportsDir) {}

PerfSummary PerfDashboard::compute() {
    RtoRpoAnalyzer analyzer(dir);
    auto runs = analyzer.analyzeAll();

    PerfSummary s{};
    s.totalRuns = static_cast<int>(runs.size());
    if (runs.empty()) return s;

    double sumRto = 0.0, sumRestore = 0.0;
    int pass = 0;
    std::vector<double> rtos;
    for (auto &r : runs) {
        sumRto += r.rto_ms;
        sumRestore += r.restore_ms;
        if (r.passed) pass++;
        rtos.push_back(r.rto_ms);
    }
    std::sort(rtos.begin(), rtos.end());
    s.avgRto = sumRto / runs.size();
    s.p95Rto = rtos[std::min<size_t>(rtos.size() - 1, static_cast<size_t>(rtos.size() * 0.95))];
    s.avgRestore = sumRestore / runs.size();
    s.chaosPassRate = 100.0 * pass / runs.size();

    double median = rtos[rtos.size() / 2];
    s.regressions = std::count_if(rtos.begin(), rtos.end(),
        [&](double v){ return v > median * 1.2; });
    return s;
}

bool PerfDashboard::writeJson(const std::string& path, const PerfSummary& s) {
    json j;
    j["avg_rto_ms"] = s.avgRto;
    j["p95_rto_ms"] = s.p95Rto;
    j["avg_restore_ms"] = s.avgRestore;
    j["chaos_pass_rate"] = s.chaosPassRate;
    j["total_runs"] = s.totalRuns;
    j["regressions"] = s.regressions;
    std::ofstream out(path);
    if (!out) return false;
    out << std::setw(2) << j;
    return true;
}

bool PerfDashboard::writeCsv(const std::string& path, const PerfSummary& s) {
    std::ofstream out(path);
    if (!out) return false;
    out << "avg_rto_ms,p95_rto_ms,avg_restore_ms,chaos_pass_rate,total_runs,regressions\n";
    out << s.avgRto << "," << s.p95Rto << "," << s.avgRestore << ","
        << s.chaosPassRate << "," << s.totalRuns << "," << s.regressions << "\n";
    return true;
}

void PerfDashboard::printAsciiChart(const std::vector<RunMetrics>& runs) {
    if (runs.empty()) return;
    std::cout << "\n=== ASCII RTO Trend Chart ===\n";
    double maxRto = 0;
    for (auto &r : runs) maxRto = std::max(maxRto, r.rto_ms);
    for (size_t i = 0; i < runs.size(); ++i) {
        int bar = static_cast<int>((runs[i].rto_ms / maxRto) * 50.0);
        std::cout << std::setw(2) << i << " [";
        for (int j = 0; j < bar; ++j) std::cout << "#";
        for (int j = bar; j < 50; ++j) std::cout << " ";
        std::cout << "] " << std::fixed << std::setprecision(0) << runs[i].rto_ms
                  << " ms " << (runs[i].passed ? "OK" : "FAIL") << "\n";
    }
    std::cout << "==============================\n";
}

