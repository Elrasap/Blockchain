#include "chaos/scenarioRunner.hpp"
#include <thread>

ScenarioRunner::ScenarioRunner(ChaosEngine* engine) : /*NOLINT*/ engine(engine) {}
ScenarioOutcome ScenarioRunner::run(const ChaosScenario& s) {
    engine->reset();
    auto start = std::chrono::steady_clock::now();
    auto nextStep = s.steps.begin();
    auto elapsed = std::chrono::milliseconds(0);

    while (true) {
        auto now = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        while (nextStep != s.steps.end() && elapsed >= nextStep->at) {
            engine->applyFault(nextStep->fault);
            ++nextStep;
        }

        engine->tick(std::chrono::milliseconds(100));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (nextStep == s.steps.end() && elapsed > (s.steps.empty() ? std::chrono::seconds(2) : s.steps.back().at + std::chrono::seconds(2)))
            break;
    }

    auto metrics = engine->snapshot();
    bool rtoOk = metrics.rto <= s.budgetRTO;
    bool finOk = metrics.finalityLag <= s.budgetFinalityLag;
    bool health = engine->isClusterHealthy();

    ScenarioOutcome out;
    out.name = s.name;
    out.passed = rtoOk && finOk && health;
    out.metrics = metrics;
    if (!out.passed) {
        out.reason = !rtoOk ? "RTO budget exceeded" : (!finOk ? "Finality lag budget exceeded" : "Cluster unhealthy at end");
    }
    return out;
}

