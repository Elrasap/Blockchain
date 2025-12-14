#include "security/policyEnforcer.hpp"
#include <algorithm>

PolicyEnforcer::PolicyEnforcer(){}

void PolicyEnforcer::evaluate(const std::vector<std::string>& vulns, bool integrityOk) {
    bool hasCritical=false;
    for(const auto& v: vulns){ if(v.find("CRITICAL")!=std::string::npos) {hasCritical=true; break;} }
    PolicyRule r1; r1.id="integrity"; r1.description="checksum+signature+sbom+attestation"; r1.passed=integrityOk;
    PolicyRule r2; r2.id="no_critical_vulns"; r2.description="no critical vulnerabilities present"; r2.passed=!hasCritical;
    rules.clear();
    rules.push_back(r1);
    rules.push_back(r2);
}

std::vector<PolicyRule> PolicyEnforcer::getResults() const { return rules; }

bool PolicyEnforcer::allPassed() const {
    for(const auto& r: rules) if(!r.passed) return false;
    return true;
}

