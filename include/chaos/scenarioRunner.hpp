#pragma once
#include <string>
#include "chaos/chaos_scenario.hpp"
#include "chaos/chaos_engine.hpp"

struct ScenarioOutcome {
    std::string name;
    bool passed;
    std::string reason;
    ChaosMetrics metrics;
};

class ScenarioRunner {
public:
    explicit ScenarioRunner(ChaosEngine* engine);
    ScenarioOutcome run(const ChaosScenario& s);
private:
    ChaosEngine* engine;
};

