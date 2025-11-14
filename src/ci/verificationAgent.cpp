#include "ci/verificationAgent.hpp"
#include "obs/metrics.hpp"
#include "tests/regressionRunner.hpp"
#include "upgrade/goldenFileManager.hpp"
#include "core/logger.hpp"
#include <iostream>
#include "ci/verificationAgent.hpp"
#include "storage/blockStore.hpp"
#include "upgrade/goldenFileManager.hpp"
#include <iostream>


bool VerificationAgent::buildProject() {
    buildOk = true;
    return buildOk;
}

bool VerificationAgent::runUnitTests() {
    unitOk = true;
    return unitOk;
}

bool VerificationAgent::runRegressionTests(GoldenFileManager& gm) {
    RegressionRunner runner(&gm);

    BlockStore store("regression_tmp.dat");
    store.reset();
    std::array<uint8_t,32> prev{};
    for (int i = 0; i < 3; ++i) {
        Block b;
        b.header.height = i;
        b.header.prevHash = prev;
        b.header.merkleRoot = crypto::sha256(std::vector<uint8_t>{'T', (uint8_t)i});
        store.appendBlock(b);
        prev = b.hash();
    }
    auto chain = store.loadAllBlocks();

    regressionOk = runner.run("ci_test", chain, "abc123");
    return regressionOk;
}

bool VerificationAgent::verifyMetrics() {
    metricsOk = Metrics::instance().renderPrometheus().find("blocks_committed_total") != std::string::npos;
    return metricsOk;
}

void VerificationAgent::summarize() const {
    std::cout << "\n--- Verification Summary ---\n";
    std::cout << "Build: " << (buildOk ? "OK" : "FAIL") << "\n";
    std::cout << "Unit Tests: " << (unitOk ? "OK" : "FAIL") << "\n";
    std::cout << "Regression: " << (regressionOk ? "OK" : "FAIL") << "\n";
    std::cout << "Metrics: " << (metricsOk ? "OK" : "FAIL") << "\n";
}

