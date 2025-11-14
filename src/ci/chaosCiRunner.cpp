#include "ci/chaosCiRunner.hpp"
#include <filesystem>
#include <iostream>
#include <vector>
#include "chaos/chaosEngine.hpp"
#include "chaos/scenarioRunner.hpp"
#include "upgrade/experimentReport.hpp"

namespace fs = std::filesystem;

int runChaosAndWriteReport(const std::string& outDir) {
    fs::create_directories(outDir);

    ChaosEngine engine(3);
    ScenarioRunner runner(&engine);

    ChaosScenario crashRecover{
        "Crash-Recovery",
        {
            {std::chrono::milliseconds(500),  ChaosFault{FaultType::Crash, 0, std::chrono::seconds(3), 0.0, "leader crash"}}
        },
        std::chrono::seconds(8),
        std::chrono::seconds(5)
    };

    ChaosScenario latencySpike{
        "Latency-Spike",
        {
            {std::chrono::milliseconds(300),  ChaosFault{FaultType::LatencySpike, -1, std::chrono::seconds(2), 900.0, "net congestion"}},
            {std::chrono::milliseconds(1200), ChaosFault{FaultType::LatencySpike, -1, std::chrono::seconds(1), 600.0, "route flap"}}
        },
        std::chrono::seconds(6),
        std::chrono::seconds(6)
    };

    ChaosScenario packetLoss{
        "Packet-Loss",
        {
            {std::chrono::milliseconds(400), ChaosFault{FaultType::PacketLoss, -1, std::chrono::seconds(3), 35.0, "loss burst"}}
        },
        std::chrono::seconds(10),
        std::chrono::seconds(7)
    };

    std::vector<ScenarioOutcome> outcomes;
    outcomes.push_back(runner.run(crashRecover));
    outcomes.push_back(runner.run(latencySpike));
    outcomes.push_back(runner.run(packetLoss));

    std::string outPath = fs::path(outDir) / "chaos_experiments.json";
    bool wrote = ExperimentReport::writeJson(outPath, outcomes);
    if (!wrote) {
        std::cerr << "[CI Runner] Failed to write report to " << outPath << "\n";
        return 2;
    }

    bool allPassed = true;
    for (const auto &o : outcomes) {
        if (!o.passed) allPassed = false;
    }

    if (!allPassed) {
        std::cerr << "[CI Runner] One or more chaos scenarios FAILED. See " << outPath << "\n";
        return 1;
    }

    std::cout << "[CI Runner] All chaos scenarios PASSED. Report: " << outPath << "\n";
    return 0;
}


