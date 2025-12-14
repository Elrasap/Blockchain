#pragma once
#include <string>
#include "security/securityScanner.hpp"
#include "security/policyEnforcer.hpp"
#include "security/auditReport.hpp"

class SecurityPipeline {
public:
    explicit SecurityPipeline(const std::string& releaseDir);
    bool run();
private:
    std::string dir;
    void logResult(const ScanResult& result, const PolicyEnforcer& enforcer);
};

