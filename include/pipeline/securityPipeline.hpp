#pragma once
#include <string>
#include "security/security_scanner.hpp"
#include "security/policy_enforcer.hpp"
#include "security/audit_report.hpp"

class SecurityPipeline {
public:
    explicit SecurityPipeline(const std::string& releaseDir);
    bool run();
private:
    std::string dir;
    void logResult(const ScanResult& result, const PolicyEnforcer& enforcer);
};

