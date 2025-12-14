#include "upgrade/experimentReport.hpp"
#include <fstream>

bool ExperimentReport::writeJson(const std::string& path, const std::vector<ScenarioOutcome>& outcomes) {
    std::ofstream out(path);
    if (!out) return false;
    out << "{\n  \"experiments\":[\n";
    for (size_t i = 0; i < outcomes.size(); ++i) {
        const auto& o = outcomes[i];
        out << "    {\"name\":\"" << o.name << "\","
            << "\"passed\":" << (o.passed ? "true" : "false") << ","
            << "\"reason\":\"" << o.reason << "\","
            << "\"metrics\":{"
            << "\"crashes\":" << o.metrics.crashes << ","
            << "\"maxLatencyMs\":" << o.metrics.maxLatencyMs << ","
            << "\"packetLossPct\":" << o.metrics.packetLossPct << ","
            << "\"peerDrops\":" << o.metrics.peerDrops << ","
            << "\"finalityLagMs\":" << o.metrics.finalityLag.count() << ","
            << "\"rtoMs\":" << o.metrics.rto.count()
            << "}}";
        if (i + 1 < outcomes.size()) out << ",\n"; else out << "\n";
    }
    out << "  ]\n}\n";
    return true;
}

