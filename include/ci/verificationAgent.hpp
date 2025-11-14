#pragma once
#include <string>
#include "tests/regressionRunner.hpp"
#include "obs/metrics.hpp"
#include "core/logger.hpp"

class VerificationAgent {
public:
    bool buildProject();
    bool runUnitTests();
    bool runRegressionTests(GoldenFileManager& gm);
    bool verifyMetrics();
    void summarize() const;
private:
    bool buildOk = false;
    bool unitOk = false;
    bool regressionOk = false;
    bool metricsOk = false;
};

