#pragma once
#include <string>
#include <array>
#include "upgrade/golden_file_manager.hpp"
#include "upgrade/schema_registry.hpp"
#include "upgrade/state_validator.hpp"

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

