#include "analytics/rto_rpo_analyzer.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

RtoRpoAnalyzer::RtoRpoAnalyzer(const std::string& reportsDir) : dir(reportsDir) {}

std::vector<RunMetrics> RtoRpoAnalyzer::analyzeAll() {
    std::vector<RunMetrics> runs;
    if (!fs::exists(dir) || !fs::is_directory(dir)) return runs;

    for (auto &p : fs::directory_iterator(dir)) {
        if (!p.is_regular_file()) continue;
        auto name = p.path().filename().string();
        if (name.rfind("recovery", 0) != 0 && name.find("recovery_") != 0 && name.find("recovery") == std::string::npos) continue;
        RunMetrics m;
        if (parseFile(p.path().string(), m)) runs.push_back(m);
    }
    return runs;
}

bool RtoRpoAnalyzer::parseFile(const std::string& path, RunMetrics& out) {
    std::ifstream in(path);
    if (!in) return false;
    json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }

    out.filename = fs::path(path).filename().string();
    out.passed = j.value("passed", false);
    out.rto_ms = out.snapshot_ms = out.restore_ms = out.verify_ms = 0.0;

    if (!j.contains("timeline") || !j["timeline"].is_array()) return true;

    std::time_t crash_start = 0;
    std::time_t restore_end = 0;
    std::time_t snap_start = 0, snap_end = 0;
    std::time_t restore_start = 0, verify_start = 0, verify_end = 0;

    for (auto &entry : j["timeline"]) {
        std::string name = entry.value("name", "");
        std::string startS = entry.value("start", "");
        std::string endS = entry.value("end", "");
        std::time_t startT = 0, endT = 0;
        bool okStart = parseCtime(startS, startT);
        bool okEnd = parseCtime(endS, endT);

        if (!okStart || !okEnd) {
            // try numeric fields if present
            if (entry.contains("start_ts") && entry.contains("end_ts")) {
                startT = (std::time_t) entry.value("start_ts", 0LL);
                endT = (std::time_t) entry.value("end_ts", 0LL);
                okStart = okEnd = (startT != 0 && endT != 0);
            }
        }

        if (!okStart || !okEnd) continue; // skip unparsable entries

        if (name == "ClusterCrash") {
            crash_start = startT;
        } else if (name == "Snapshot") {
            snap_start = startT;
            snap_end = endT;
            out.snapshot_ms = diffMs(snap_start, snap_end);
        } else if (name == "Restore") {
            restore_start = startT;
            restore_end = endT;
            out.restore_ms = diffMs(restore_start, restore_end);
        } else if (name == "VerifyState") {
            verify_start = startT;
            verify_end = endT;
            out.verify_ms = diffMs(verify_start, verify_end);
        }
    }

    if (crash_start != 0 && restore_end != 0) out.rto_ms = diffMs(crash_start, restore_end);
    return true;
}

double RtoRpoAnalyzer::diffMs(std::time_t a, std::time_t b) {
    // returns milliseconds from a->b (b - a)
    return static_cast<double>((b - a) * 1000.0);
}

bool RtoRpoAnalyzer::parseCtime(const std::string& s, std::time_t& out) {
    if (s.empty()) return false;
    // Trim possible trailing newline
    std::string in = s;
    while (!in.empty() && (in.back() == '\n' || in.back() == '\r')) in.pop_back();

    std::istringstream ss(in);
    std::tm tm{};
#if defined(_MSC_VER)
    // MSVC: use _strptime if available - but generally std::get_time works on GCC/Clang
    // Fallback: attempt parsing known formats manually (not implemented)
    if (!(ss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y"))) return false;
#else
    if (!(ss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y"))) {
        // Some ctime strings use two-char day with leading space, ensure parsing works:
        // try alternative "%c" which is locale-dependent
        ss.clear();
        ss.str(in);
        if (!(ss >> std::get_time(&tm, "%c"))) return false;
    }
#endif
    out = std::mktime(&tm);
    return out != -1;
}

bool RtoRpoAnalyzer::writeSummaryJson(const std::string& outPath, const std::vector<RunMetrics>& runs) const {
    json root;
    root["report_count"] = runs.size();
    double sum_rto = 0.0;
    std::vector<double> rtos;
    int passed = 0;
    for (auto &r : runs) {
        if (r.passed) passed++;
        rtos.push_back(r.rto_ms);
        sum_rto += r.rto_ms;
    }
    double avg = runs.empty() ? 0.0 : sum_rto / runs.size();
    std::sort(rtos.begin(), rtos.end());
    double p50 = rtos.empty() ? 0.0 : rtos[rtos.size()/2];
    double p95 = rtos.empty() ? 0.0 : rtos[std::min<size_t>(rtos.size()-1, (size_t)std::ceil(rtos.size()*0.95)-1)];

    root["summary"]["avg_rto_ms"] = avg;
    root["summary"]["p50_rto_ms"] = p50;
    root["summary"]["p95_rto_ms"] = p95;
    root["summary"]["passed_count"] = passed;
    root["summary"]["total_count"] = (int)runs.size();

    root["runs"] = json::array();
    for (auto &r : runs) {
        json rr;
        rr["file"] = r.filename;
        rr["passed"] = r.passed;
        rr["rto_ms"] = r.rto_ms;
        rr["snapshot_ms"] = r.snapshot_ms;
        rr["restore_ms"] = r.restore_ms;
        rr["verify_ms"] = r.verify_ms;
        root["runs"].push_back(rr);
    }
    std::ofstream out(outPath);
    if (!out) return false;
    out << std::setw(2) << root;
    return true;
}

bool RtoRpoAnalyzer::writeCsv(const std::string& outPath, const std::vector<RunMetrics>& runs) const {
    std::ofstream out(outPath);
    if (!out) return false;
    out << "file,passed,rto_ms,snapshot_ms,restore_ms,verify_ms\n";
    for (auto &r : runs) {
        out << r.filename << "," << (r.passed ? "1" : "0") << "," << r.rto_ms << "," << r.snapshot_ms << "," << r.restore_ms << "," << r.verify_ms << "\n";
    }
    return true;
}

