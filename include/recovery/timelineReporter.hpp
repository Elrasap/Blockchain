#pragma once
#include <string>
#include "recovery/disaster_recovery.hpp"

class TimelineReporter {
public:
    static bool writeJson(const std::string& path, const RecoveryOutcome& outcome);
};

