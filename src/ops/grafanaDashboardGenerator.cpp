#include "ops/grafanaDashboardGenerator.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

void GrafanaDashboardGenerator::generate(const std::string& outputPath) {
    json panelRto = {
        {"type", "graph"},
        {"title", "Average Recovery Time (ms)"},
        {"targets", {{{"expr", "blockchain_avg_rto_ms"}}}},
        {"lines", true}, {"linewidth", 2},
        {"fill", 1},
        {"gridPos", {{"h", 8}, {"w", 12}, {"x", 0}, {"y", 0}}}
    };

    json panelPassRate = {
        {"type", "graph"},
        {"title", "Pass Rate (%)"},
        {"targets", {{{"expr", "blockchain_pass_rate_percent"}}}},
        {"lines", true}, {"linewidth", 2},
        {"fill", 1},
        {"gridPos", {{"h", 8}, {"w", 12}, {"x", 12}, {"y", 0}}}
    };

    json panelIntegrity = {
        {"type", "stat"},
        {"title", "Integrity OK"},
        {"targets", {{{"expr", "blockchain_integrity_ok"}}}},
        {"gridPos", {{"h", 4}, {"w", 6}, {"x", 0}, {"y", 8}}}
    };

    json panelPerformance = {
        {"type", "stat"},
        {"title", "Performance OK"},
        {"targets", {{{"expr", "blockchain_performance_ok"}}}},
        {"gridPos", {{"h", 4}, {"w", 6}, {"x", 6}, {"y", 8}}}
    };

    json panelAnomalies = {
        {"type", "graph"},
        {"title", "Detected Anomalies"},
        {"targets", {{{"expr", "blockchain_anomalies_total"}}}},
        {"lines", true},
        {"fill", 2},
        {"gridPos", {{"h", 8}, {"w", 12}, {"x", 0}, {"y", 12}}}
    };

    json dashboard = {
        {"id", nullptr},
        {"uid", "blockchain-observability"},
        {"title", "Blockchain Observability Dashboard"},
        {"schemaVersion", 37},
        {"version", 1},
        {"refresh", "10s"},
        {"time", {{"from", "now-1h"}, {"to", "now"}}},
        {"panels", json::array({panelRto, panelPassRate, panelIntegrity, panelPerformance, panelAnomalies})}
    };

    json wrapper = {
        {"dashboard", dashboard},
        {"overwrite", true}
    };

    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "[GrafanaDashboardGenerator] Could not write to " << outputPath << "\n";
        return;
    }
    out << wrapper.dump(2);
    std::cout << "[GrafanaDashboardGenerator] Dashboard written to " << outputPath << "\n";
}

