#pragma once
#include <string>
#include <vector>
#include <array>
#include "upgrade/goldenFileManager.hpp"
#include "upgrade/schemaRegistry.hpp"
#include "upgrade/stateValidator.hpp"
#include "tests/regressionReporter.hpp"

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

