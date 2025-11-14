#pragma once
#include <vector>
#include "recovery/recoveryStep.hpp"

struct RecoveryOutcome {
    bool passed;
    std::string reason;
    std::vector<RecoveryStep> timeline;
};

class DisasterRecovery {
public:
    explicit DisasterRecovery(int nodes);
    RecoveryOutcome run();
private:
    int nodeCount;
    bool simulateCrash();
    bool createSnapshots();
    bool restoreFromSnapshots();
    bool verifyConsistency();
};
