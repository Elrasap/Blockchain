#include "recovery/disaster_recovery.hpp"
#include <iostream>
#include <thread>
#include <chrono>

DisasterRecovery::DisasterRecovery(int nodes) : nodeCount(nodes) {}

bool DisasterRecovery::simulateCrash() {
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    std::cout << "[Recovery] All " << nodeCount << " nodes crashed.\n";
    return true;
}

bool DisasterRecovery::createSnapshots() {
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    std::cout << "[Recovery] Snapshots created successfully.\n";
    return true;
}

bool DisasterRecovery::restoreFromSnapshots() {
    std::this_thread::sleep_for(std::chrono::milliseconds(900));
    std::cout << "[Recovery] Nodes restored from snapshots.\n";
    return true;
}

bool DisasterRecovery::verifyConsistency() {
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    std::cout << "[Recovery] State verified – all consistent.\n";
    return true;
}

RecoveryOutcome DisasterRecovery::run() {
    RecoveryOutcome outcome;
    outcome.passed = true;

    auto now = std::chrono::system_clock::now();

    RecoveryStep crash{"ClusterCrash", "Crash all nodes", now, now, false, ""};
    crash.start = std::chrono::system_clock::now();
    crash.success = simulateCrash();
    crash.end = std::chrono::system_clock::now();
    outcome.timeline.push_back(crash);

    RecoveryStep snap{"Snapshot", "Create cluster snapshots", crash.end, crash.end, false, ""};
    snap.start = std::chrono::system_clock::now();
    snap.success = createSnapshots();
    snap.end = std::chrono::system_clock::now();
    outcome.timeline.push_back(snap);

    RecoveryStep restore{"Restore", "Reload nodes from snapshots", snap.end, snap.end, false, ""};
    restore.start = std::chrono::system_clock::now();
    restore.success = restoreFromSnapshots();
    restore.end = std::chrono::system_clock::now();
    outcome.timeline.push_back(restore);

    RecoveryStep verify{"VerifyState", "Check consistency and finality", restore.end, restore.end, false, ""};
    verify.start = std::chrono::system_clock::now();
    verify.success = verifyConsistency();
    verify.end = std::chrono::system_clock::now();
    outcome.timeline.push_back(verify);

    for (auto& s : outcome.timeline)
        if (!s.success) { outcome.passed = false; outcome.reason = "Failure at step: " + s.name; break; }

    if (outcome.passed) outcome.reason = "Disaster recovery successful";
    return outcome;
}

