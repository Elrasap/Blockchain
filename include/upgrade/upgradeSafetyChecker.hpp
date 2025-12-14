#pragma once
#include <string>
#include <array>
#include "upgrade/goldenFileManager.hpp"
#include "upgrade/schemaRegistry.hpp"
#include "upgrade/stateValidator.hpp"

struct SafetyReport {
    std::string from_version;
    std::string to_version;
    bool schema_ok;
    bool state_ok;
    bool overall_ok;
};

class UpgradeSafetyChecker {
public:
    static SafetyReport checkUpgrade(const std::string& fromVer,
                                     const std::string& toVer,
                                     GoldenFileManager& gm,
                                     const std::array<uint8_t,32>& currentRoot,
                                     const std::string& currentSchema);
};

