#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <openssl/sha.h>
#include <nlohmann/json.hpp>

#include "ops/reliabilityGuard.hpp"
#include "analytics/rtoRpoAnalyzer.hpp"
#include "analytics/trendAnalyzer.hpp"
#include "web/dashboardServer.hpp"
#include "metrics/metricsServer.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

/* -------------------------------------------------------
 *  Hilfsstruktur für Selftests
 * -----------------------------------------------------*/
struct Check {
    std::string name;
    bool ok;
    std::string info;
};

static void printReport(const std::vector<Check>& checks) {
    int pass = 0, fail = 0;
    std::cout << "\n=== E2E SELFTEST REPORT ===\n";
    for (const auto& c : checks) {
        std::cout << std::left << std::setw(30) << c.name << " : "
                  << (c.ok ? "OK    " : "FAIL  ")
                  << " : " << c.info << "\n";
        if (c.ok) ++pass; else ++fail;
    }
    std::cout << "--------------------------------------\n";
    std::cout << "PASS=" << pass << " FAIL=" << fail << "\n";
    std::cout << (fail == 0 ? "SELFTEST: ALL CHECKS PASSED\n"
                            : "SELFTEST: SOME CHECKS FAILED\n");
}

/* -------------------------------------------------------
 *  SHA256 einer Datei (Hex)
 *  (nutzt OpenSSL – du hast es ohnehin im Projekt)
 * -----------------------------------------------------*/
static std::string sha256File(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    char buf[4096];
    while (in.good()) {
        in.read(buf, sizeof(buf));
        std::streamsize n = in.gcount();
        if (n > 0) {
            SHA256_Update(&ctx, buf, static_cast<size_t>(n));
        }
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return oss.str();
}

/* -------------------------------------------------------
 *  1) Binary + Checksum
 *     Erwartung: du startest in Projekt-Root:
 *       ./build/blockchain_node
 *     Dann:
 *       - build/blockchain_node existiert
 *       - blockchain_node.sha256 existiert
 * -----------------------------------------------------*/
static Check checkBinaryAndChecksum() {
    if (!fs::exists("build/blockchain_node")) {
        return {"binary+checksum", false, "missing build/blockchain_node"};
    }
    if (!fs::exists("blockchain_node.sha256")) {
        return {"binary+checksum", false, "missing blockchain_node.sha256"};
    }

    std::string expected;
    {
        std::ifstream in("blockchain_node.sha256");
        in >> expected;
    }
    auto actual = sha256File("build/blockchain_node");
    if (actual.empty()) {
        return {"binary+checksum", false, "cannot read build/blockchain_node"};
    }
    if (expected != actual) {
        return {"binary+checksum", false, "sha256 mismatch"};
    }
    return {"binary+checksum", true, "ok"};
}

/* -------------------------------------------------------
 *  2) Dockerfile.verify
 * -----------------------------------------------------*/
static Check checkDockerfileVerify() {
    if (!fs::exists("Dockerfile.verify")) {
        return {"Dockerfile.verify content", false, "missing"};
    }
    std::ifstream f("Dockerfile.verify");
    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());

    bool ok =
        content.find("python:3.11-slim") != std::string::npos &&
        content.find("verify_manifest.py") != std::string::npos;

    return {"Dockerfile.verify content", ok, ok ? "ok" : "content mismatch"};
}

/* -------------------------------------------------------
 *  3) GitHub Workflow
 * -----------------------------------------------------*/
static Check checkWorkflow() {
    if (!fs::exists(".github/workflows/build.yml")) {
        return {".github/workflows/build.yml", false, "missing"};
    }
    return {"workflow Build and Verify", true, "ok"};
}

/* -------------------------------------------------------
 *  4) verify_manifest.py
 * -----------------------------------------------------*/
static Check checkVerifyScript() {
    if (!fs::exists("scripts/verify_manifest.py")) {
        return {"verify_manifest.py", false, "missing"};
    }
    return {"verify_manifest.py", true, "ok"};
}

/* -------------------------------------------------------
 *  5) Release Manifest + Keys
 * -----------------------------------------------------*/
static Check checkManifestFiles() {
    bool man  = fs::exists("release_manifest.json");
    bool key  = fs::exists("release/ed25519.key");
    bool pub  = fs::exists("release/ed25519.pub");
    bool all  = man && key && pub;
    return {"release manifest files", all, all ? "ok" : "missing"};
}

/* -------------------------------------------------------
 *  6) Reports-Verzeichnis
 * -----------------------------------------------------*/
static Check checkReportsDir() {
    if (!fs::exists("reports")) {
        return {"reports dir", false, "missing"};
    }
    std::size_t count = 0;
    for (auto& p : fs::directory_iterator("reports")) {
        if (p.is_regular_file()) ++count;
    }
    return {"reports dir", count > 0, "files=" + std::to_string(count)};
}

/* -------------------------------------------------------
 *  7) RtoRpoAnalyzer – liest Reports
 * -----------------------------------------------------*/
static Check checkRtoAnalyzer() {
    try {
        RtoRpoAnalyzer analyzer("reports");
        auto runs = analyzer.analyzeAll();
        if (runs.empty()) {
            return {"RtoRpoAnalyzer.analyzeAll", false, "no runs"};
        }
        double avg = 0.0;
        for (auto& r : runs) avg += r.rto_ms;
        avg /= runs.size();
        return {"RtoRpoAnalyzer.analyzeAll", true,
                "runs=" + std::to_string(runs.size()) +
                " avg_rto_ms=" + std::to_string(avg)};
    } catch (const std::exception& e) {
        return {"RtoRpoAnalyzer.analyzeAll", false, std::string("exception: ") + e.what()};
    } catch (...) {
        return {"RtoRpoAnalyzer.analyzeAll", false, "unknown exception"};
    }
}

/* -------------------------------------------------------
 *  8) TrendAnalyzer – History / Daily-Stats
 * -----------------------------------------------------*/
static Check checkTrendAnalyzer() {
    try {
        TrendAnalyzer t("history.db");
        auto data = t.loadDaily();
        auto summary = t.computeSummary(data);
        std::ostringstream oss;
        oss << "days=" << data.size()
            << " mean_rto_ms=" << summary.meanRto
            << " slope=" << summary.slope
            << " regressions=" << summary.regressions;
        return {"TrendAnalyzer", true, oss.str()};
    } catch (const std::exception& e) {
        return {"TrendAnalyzer", false, std::string("exception: ") + e.what()};
    } catch (...) {
        return {"TrendAnalyzer", false, "unknown exception"};
    }
}

/* -------------------------------------------------------
 *  9) ReliabilityGuard – Gesamtzustand
 * -----------------------------------------------------*/
static Check checkReliabilityGuard() {
    try {
        ReliabilityGuard guard("reports", "build/blockchain_node");
        auto s = guard.evaluate(8000.0, 95.0, 1);

        std::ostringstream oss;
        oss << "integrity=" << s.integrityOk
            << " perf=" << s.perfOk
            << " chaos=" << s.chaosOk
            << " forecast=" << s.forecastOk
            << " avgRto=" << s.avgRto
            << " passRate=" << s.passRate
            << " anomalies=" << s.anomalies;

        bool ok = s.integrityOk && s.perfOk && s.forecastOk;
        return {"ReliabilityGuard.evaluate", ok, oss.str()};
    } catch (const std::exception& e) {
        return {"ReliabilityGuard.evaluate", false, std::string("exception: ") + e.what()};
    } catch (...) {
        return {"ReliabilityGuard.evaluate", false, "unknown exception"};
    }
}

/* -------------------------------------------------------
 * 10) grafana_dashboard.json – Struktur
 * -----------------------------------------------------*/
static Check checkGrafanaDashboard() {
    if (!fs::exists("grafana_dashboard.json")) {
        return {"grafana_dashboard.json structure", false, "missing"};
    }
    try {
        std::ifstream f("grafana_dashboard.json");
        json j; f >> j;
        bool ok = j.contains("title");
        return {"grafana_dashboard.json structure", ok, ok ? "ok" : "missing title"};
    } catch (const std::exception& e) {
        return {"grafana_dashboard.json structure", false, std::string("parse error: ") + e.what()};
    } catch (...) {
        return {"grafana_dashboard.json structure", false, "unknown parse error"};
    }
}

/* -------------------------------------------------------
 *  MAIN
 *  - Führt alle Checks aus
 *  - Startet MetricsServer + DashboardServer
 * -----------------------------------------------------*/
int main() {
    // ------------------- SELFTESTS -------------------
    std::vector<Check> checks;
    checks.push_back(checkBinaryAndChecksum());
    checks.push_back(checkDockerfileVerify());
    checks.push_back(checkWorkflow());
    checks.push_back(checkVerifyScript());
    checks.push_back(checkManifestFiles());
    checks.push_back(checkReportsDir());
    checks.push_back(checkRtoAnalyzer());
    checks.push_back(checkTrendAnalyzer());
    checks.push_back(checkReliabilityGuard());
    checks.push_back(checkGrafanaDashboard());

    printReport(checks);

    // ------------------- SERVICES STARTEN -------------------
    // Metrics (Prometheus /health /metrics)
    MetricsServer metricsServer(9100);
    metricsServer.start();

    // Dashboard + DnD-UI – läuft in eigenem Thread
    DashboardServer dashboard(8080, "reports", "build/blockchain_node");
    std::thread dashThread([&]() {
        dashboard.start();   // blockierend
    });

    std::cout << "\n=== NODE STARTED ===\n";
    std::cout << "Metrics:   http://localhost:9100/metrics\n";
    std::cout << "Dashboard: http://localhost:8080/\n";
    std::cout << "DnD-UI:    im Dashboard unter / (Characters, Mobs, Encounters)\n";
    std::cout << "Drücke STRG+C zum Beenden.\n";

    // Node läuft; hier kein weiterer Code – wir warten auf das Dashboard
    dashThread.join();
    metricsServer.stop();
    return 0;
}

