#pragma once
#include <string>
#include "chaos/chaosScenario.hpp"
#include "chaos/chaosEngine.hpp"

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

