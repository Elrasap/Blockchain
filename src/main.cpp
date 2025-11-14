#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

#include "storage/historyStore.hpp"
#include "ops/reliabilityGuard.hpp"
#include "analytics/rtoRpoAnalyzer.hpp"
#include "metrics/metricsServer.hpp"

struct Check {
    std::string name;
    bool ok;
    std::string msg;
};

// -------------------------------------------------------
//  HISTORYSTORE SELFTEST
// -------------------------------------------------------
Check checkHistoryStore() {
    HistoryStore hs("history.db");

    if (!hs.init())
        return {"HistoryStore init", false, "init() failed"};

    // Beispiel-RTO-Eintrag
    RtoRecord rec;
    rec.filename = "selftest.bin";
    rec.rto_ms = 10.0;
    rec.restore_ms = 5.0;
    rec.passed = true;

    hs.insertRtoRecords({rec});

    // Beispiel-Reliability
    ReliabilityStatus rs;
    rs.integrityOk = true;
    rs.perfOk = true;
    rs.chaosOk = true;
    rs.forecastOk = true;
    rs.avgRto = 10.0;
    rs.passRate = 1.0;
    rs.anomalies = 0;

    hs.insertReliability(rs);

    auto r = hs.loadRecentRto(10);
    if (r.empty())
        return {"HistoryStore IO", false, "recent=0"};

    return {"HistoryStore IO", true, "recent=" + std::to_string(r.size())};
}

// -------------------------------------------------------
//  RTO/RPO ANALYZER SELFTEST
// -------------------------------------------------------
Check checkRtoAnalyzer() {
    // reports directory vorbereiten
    std::filesystem::create_directories("reports");

    // gültige Datei erzeugen
    {
        std::ofstream out("reports/recovery_selftest.json");
        out << R"({
  "passed": true,
  "timeline": [
    { "name": "ClusterCrash", "start_ts": 1700000000, "end_ts": 1700000001 },
    { "name": "Snapshot",     "start_ts": 1700000001, "end_ts": 1700000010 },
    { "name": "Restore",      "start_ts": 1700000010, "end_ts": 1700000020 },
    { "name": "VerifyState",  "start_ts": 1700000020, "end_ts": 1700000030 }
  ]
})";
    }

    RtoRpoAnalyzer analyzer("reports");
    auto all = analyzer.analyzeAll();

    if (all.empty())
        return {"RtoRpoAnalyzer.analyzeAll", false, "no runs"};

    return {"RtoRpoAnalyzer.analyzeAll", true,
            "runs=" + std::to_string(all.size())};
}

// -------------------------------------------------------
//  RELIABILITY GUARD SELFTEST
// -------------------------------------------------------
Check checkReliabilityGuard() {
    ReliabilityGuard guard("reports", "blockchain_node");

    auto r = guard.evaluate(1000.0, 0.0, 100);

    if (!r.integrityOk)
        return {"ReliabilityGuard.evaluate", false, "integrity=0"};

    return {"ReliabilityGuard.evaluate", true, "OK"};
}

// -------------------------------------------------------
//  RUN ALL SELFTESTS
// -------------------------------------------------------
void runSelftests() {
    std::vector<Check> checks;

    checks.push_back(checkHistoryStore());
    checks.push_back(checkRtoAnalyzer());
    checks.push_back(checkReliabilityGuard());

    int pass = 0, fail = 0;

    std::cout << "\n=== E2E SELFTEST REPORT ===\n";
    for (auto &c : checks) {
        if (c.ok) {
            std::cout << c.name << " : PASS   : " << c.msg << "\n";
            pass++;
        } else {
            std::cout << c.name << " : FAIL   : " << c.msg << "\n";
            fail++;
        }
    }

    std::cout << "--------------------------------------\n";
    std::cout << "PASS=" << pass << " FAIL=" << fail << "\n";
}

// -------------------------------------------------------
//  MAIN
// -------------------------------------------------------
int main() {
    std::cout << "[Node] Starting blockchain node...\n";

    runSelftests();

    std::thread([](){
        MetricsServer server(9100);
        std::cout << "[MetricsServer] Running on http://localhost:9100\n";
        server.start();
    }).detach();

    std::cout << "[Dashboard] running on http://localhost:8080\n";
    std::cout << "Node running. CTRL+C to exit.\n";

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(1));
}

