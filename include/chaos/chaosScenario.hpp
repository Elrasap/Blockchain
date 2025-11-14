#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "chaos/chaos_fault.hpp"

struct ScenarioStep {
    std::chrono::milliseconds at;  // Zeitpunkt relativ zum Start
    ChaosFault fault;
};

struct ChaosScenario {
    std::string name;
    std::vector<ScenarioStep> steps;
    std::chrono::milliseconds budgetRTO{std::chrono::seconds(10)};
    std::chrono::milliseconds budgetFinalityLag{std::chrono::seconds(5)};
};

