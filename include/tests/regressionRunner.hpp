#pragma once
#include <string>
#include <vector>
#include <array>
#include "upgrade/golden_file_manager.hpp"
#include "upgrade/schema_registry.hpp"
#include "upgrade/state_validator.hpp"
#include "tests/regression_reporter.hpp"

class RegressionRunner {
public:
    RegressionRunner(GoldenFileManager* gm);
    bool run(const std::string& version,
             const std::vector<Block>& chain,
             const std::string& schemaHash);
    RegressionReporter generateReport() const;
private:
    GoldenFileManager* gm;
    bool passed = false;
    std::string testedVersion;
    std::string diff;
};

