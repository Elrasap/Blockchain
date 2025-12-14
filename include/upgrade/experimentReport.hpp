#pragma once
#include <string>
#include <vector>
#include "chaos/scenarioRunner.hpp"

class ExperimentReport {
public:
    static bool writeJson(const std::string& path, const std::vector<ScenarioOutcome>& outcomes);
};

