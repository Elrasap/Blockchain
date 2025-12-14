#pragma once
#include <string>
#include <vector>

struct PolicyRule {
    std::string id;
    std::string description;
    bool passed;
};

class PolicyEnforcer {
public:
    PolicyEnforcer();
    void evaluate(const std::vector<std::string>& vulns, bool integrityOk);
    std::vector<PolicyRule> getResults() const;
    bool allPassed() const;
private:
    std::vector<PolicyRule> rules;
};

