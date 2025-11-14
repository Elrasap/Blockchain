#pragma once
#include <string>
#include "recovery/disasterRecovery.hpp"

class TimelineReporter {
public:
    static bool writeJson(const std::string& path, const RecoveryOutcome& outcome);
};

