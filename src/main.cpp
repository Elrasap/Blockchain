#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

#include <nlohmann/json.hpp>   // <-- WICHTIG: JSON-Parser einbinden!

#include "ops/reliabilityGuard.hpp"
#include "analytics/rtoRpoAnalyzer.hpp"
#include "storage/historyStore.hpp"

namespace fs = std::filesystem;

struct Check {
    std::string name;
    bool ok;
    std::string detail;
};

static bool fileExists(const fs::path& p) {
    std::error_code ec;
    return fs::exists(p, ec);
}

static std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool containsAll(const std::string& txt, const std::vector<std::string>& needles) {
    for (auto& n : needles) {
        if (txt.find(n) == std::string::npos) return false;
    }
    return true;
}

static void addCheck(std::vector<Check>& list, const std::string& name, bool ok, const std::string& detail) {
    list.push_back({name, ok, detail});
}

// ============================================================================
// Prüft Build-Artefakte
// ============================================================================
static Check checkBinaryAndSha() {
    fs::path bin = "./build/blockchain_node";
    fs::path sha = "blockchain_node.sha256";

    if (!fileExists(bin)) return {"binary/build", false, "missing ./build/blockchain_node"};
    if (!fileExists(sha)) return {"binary/sha256", false, "missing blockchain_node.sha256"};

    std::string line = readFile(sha);
    bool ok = !line.empty() && line.find("blockchain_node") != std::string::npos;

    return {"binary+checksum", ok, ok ? "ok" : "invalid sha file"};
}

// ============================================================================
// Dockerfile.verify
// ============================================================================
static Check checkDockerfileVerify() {
    fs::path p = "Dockerfile.verify";
    if (!fileExists(p)) return {"Dockerfile.verify", false, "missing"};

    auto t = readFile(p);
    bool ok = containsAll(t, {
        "# syntax=docker/dockerfile:1",
        "FROM python:3.11-slim AS base",
        "WORKDIR /app",
        "COPY scripts/verify_manifest.py",
        "RUN pip install --no-cache-dir pynacl",
        "ENTRYPOINT [\"python3\", \"/app/verify_manifest.py\"]",
        "CMD [\"--help\"]"
    });

    return {"Dockerfile.verify content", ok, ok ? "ok" : "content mismatch"};
}

// ============================================================================
// GitHub Workflow
// ============================================================================
static Check checkWorkflowBuildVerify() {
    fs::path p = ".github/workflows/build.yml";
    if (!fileExists(p)) return {".github/workflows/build.yml", false, "missing"};

    auto t = readFile(p);
    bool ok = containsAll(t, {
        "name: Build and Verify",
        "actions/checkout@v4",
        "cmake -B build -DCMAKE_BUILD_TYPE=Release",
        "cmake --build build",
        "sha256sum build/blockchain_node",
        "upload-artifact"
    });

    return {"workflow Build and Verify", ok, ok ? "ok" : "content mismatch"};
}

// ============================================================================
// Python verify_manifest.py
// ============================================================================
static Check checkPythonVerifier() {
    fs::path p = "scripts/verify_manifest.py";
    if (!fileExists(p)) return {"scripts/verify_manifest.py", false, "missing"};

    auto t = readFile(p);
    bool ok = containsAll(t, {
        "from nacl.signing import VerifyKey",
        "hashlib",
        "def verify_manifest",
        "if __name__ == \"__main__\""
    });

    return {"verify_manifest.py", ok, ok ? "ok" : "content mismatch"};
}

// ============================================================================
// Release Manifest + Public Key
// ============================================================================
static Check checkReleaseManifestFiles() {
    bool man = fileExists("release_manifest.json");
    bool key = fileExists("public.key");

    if (!man && !key) return {"release manifest + key", false, "both missing"};
    if (!man) return {"release_manifest.json", false, "missing"};
    if (!key) return {"public.key", false, "missing"};

    return {"release manifest files", true, "ok"};
}

// ============================================================================
// Reports
// ============================================================================
static Check checkReportsDir() {
    fs::path d = "./reports";
    if (!fileExists(d)) return {"./reports", false, "missing"};

    std::size_t count = 0;
    for (auto& e : fs::directory_iterator(d)) count++;

    bool ok = count > 0;
    std::ostringstream ss; ss << "files=" << count;

    return {"reports dir", ok, ok ? ss.str() : "empty"};
}

// ============================================================================
// RTO Analyzer
// ============================================================================
static Check checkRtoAnalyzer() {
    RtoRpoAnalyzer a("./reports");
    auto runs = a.analyzeAll();
    bool ok = !runs.empty();

    std::ostringstream ss; ss << "runs=" << runs.size();
    return {"RtoRpoAnalyzer.analyzeAll", ok, ok ? ss.str() : "no runs"};
}

// ============================================================================
// HistoryStore
// ============================================================================
static Check checkHistoryStore() {
    HistoryStore hs("history.db");
    hs.init();

    ReliabilityStatus rs{};
    rs.integrityOk = true;
    rs.perfOk = true;
    rs.chaosOk = true;
    rs.forecastOk = true;
    rs.avgRto = 5000;
    rs.passRate = 99;
    rs.anomalies = 0;

    bool a = hs.insertReliability(rs);

    RunMetrics r;
    r.filename = "debug_sanity.json";
    r.rto_ms = 5000;
    r.restore_ms = 2000;
    r.passed = true;

    bool b = hs.insertRtoRecords({r});
    auto recent = hs.loadRecentRto(5);

    bool ok = a && b && !recent.empty();

    std::ostringstream ss;
    ss << "insertReliability=" << a << " insertRtoRecords=" << b << " recent=" << recent.size();

    return {"HistoryStore IO", ok, ss.str()};
}

// ============================================================================
// ReliabilityGuard
// ============================================================================
static Check checkReliabilityGuard() {
    ReliabilityGuard g("./reports", "./build/blockchain_node");
    auto st = g.evaluate(8000.0, 95.0, 2);

    bool ok = st.integrityOk;

    std::ostringstream ss;
    ss << "integrity=" << st.integrityOk
       << " perf=" << st.perfOk
       << " chaos=" << st.chaosOk
       << " forecast=" << st.forecastOk
       << " avgRto=" << st.avgRto
       << " passRate=" << st.passRate
       << " anomalies=" << st.anomalies;

    return {"ReliabilityGuard.evaluate", ok, ss.str()};
}

// ============================================================================
// grafana_dashboard.json
// ============================================================================
static Check checkGrafanaDashboardJson() {
    fs::path p = "grafana_dashboard.json";
    if (!fileExists(p)) return {"grafana_dashboard.json", false, "missing"};

    try {
        auto txt = readFile(p);
        auto j = nlohmann::json::parse(txt);

        bool ok = j.contains("dashboard") && j["dashboard"].contains("panels");
        return {"grafana_dashboard.json structure", ok, ok ? "ok" : "invalid structure"};
    }
    catch (...) {
        return {"grafana_dashboard.json structure", false, "json parse error"};
    }
}

// ============================================================================
// Dashboard /api/trend Route (Source verify)
// ============================================================================
static Check checkDashboardTrendRouteSource() {
    fs::path p = "src/web/dashboardServer.cpp";
    if (!fileExists(p)) return {"dashboardServer.cpp", false, "missing"};

    auto t = readFile(p);
    bool ok = containsAll(t, {
        "svr.Get(\"/api/trend\"",
        "TrendAnalyzer",
        "computeSummary",
        "j[\"mean_rto_ms\"]",
        "j[\"median_rto_ms\"]",
        "j[\"slope\"]"
    });

    return {"dashboard /api/trend", ok, ok ? "ok" : "content mismatch"};
}

// ============================================================================
// Name-Check: derBaum (korrekt) vs der_baum (falsch)
// ============================================================================
static Check checkDerBaumNaming() {
    std::vector<fs::path> bad;
    std::vector<fs::path> good;

    for (auto& it : fs::recursive_directory_iterator(".")) {
        if (!it.is_regular_file()) continue;

        auto name = it.path().filename().string();
        if (name == "der_baum") bad.push_back(it.path());
        if (name == "derBaum") good.push_back(it.path());
    }

    bool ok = bad.empty();

    std::ostringstream ss;
    if (!bad.empty()) {
        ss << "Forbidden files:";
        for (auto& p : bad) ss << " " << p.string();
    }
    return {"derBaum naming", ok, ss.str().empty() ? "ok" : ss.str()};
}

// ============================================================================
// Output summary
// ============================================================================
static void printReport(const std::vector<Check>& checks) {
    int pass = 0;
    int fail = 0;

    std::cout << "\n=== E2E SELFTEST REPORT ===\n";

    for (auto& c : checks) {
        if (c.ok) pass++; else fail++;

        std::cout << std::left << std::setw(32) << c.name
                  << " : " << (c.ok ? "OK   " : "FAIL ")
                  << " : " << c.detail << "\n";
    }

    std::cout << "--------------------------------------\n";
    std::cout << "PASS=" << pass << " FAIL=" << fail << "\n";
    if (fail == 0)
        std::cout << "SELFTEST: ALL CHECKS PASSED\n";
    else
        std::cout << "SELFTEST: SOME CHECKS FAILED\n";
}

int main() {
    std::vector<Check> checks;

    checks.push_back(checkBinaryAndSha());
    checks.push_back(checkDockerfileVerify());
    checks.push_back(checkWorkflowBuildVerify());
    checks.push_back(checkPythonVerifier());
    checks.push_back(checkReleaseManifestFiles());
    checks.push_back(checkReportsDir());
    checks.push_back(checkRtoAnalyzer());
    checks.push_back(checkHistoryStore());
    checks.push_back(checkReliabilityGuard());
    checks.push_back(checkGrafanaDashboardJson());
    checks.push_back(checkDashboardTrendRouteSource());
    checks.push_back(checkDerBaumNaming());

    printReport(checks);
    return 0;
}

