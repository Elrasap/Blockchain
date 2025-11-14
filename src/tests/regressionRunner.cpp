#include "tests/regressionRunner.hpp"
#include "core/crypto.hpp"

RegressionRunner::RegressionRunner(GoldenFileManager* g) : gm(g) {}

bool RegressionRunner::run(const std::string& version,
                           const std::vector<Block>& chain,
                           const std::string& schemaHash) {
    testedVersion = version;
    auto root = StateValidator::computeStateRoot(chain);
    std::string rootHex = toHex(root);

    if (!gm->exists(version)) {
        gm->writeReference(version, root, schemaHash);
        passed = true;
        diff = "";
        return passed;
    }

    auto ref = gm->readReference(version);
    std::string refRoot = ref["state_root"];
    std::string refSchema = ref["schema"];

    bool stateOk = (refRoot == rootHex);
    bool schemaOk = (refSchema == schemaHash);

    passed = stateOk && schemaOk;

    if (passed) {
        diff = "";
        return true;
    }

    if (!stateOk) {
        diff += "state_root mismatch: expected " + refRoot + " got " + rootHex + "; ";
    }
    if (!schemaOk) {
        diff += "schema mismatch: expected " + refSchema + " got " + schemaHash + "; ";
    }

    return false;
}

RegressionReporter RegressionRunner::generateReport() const {
    RegressionReporter rep;
    rep.add({testedVersion, passed, diff});
    return rep;
}

