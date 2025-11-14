#pragma once
#include <string>
#include <vector>
#include "security/policy_enforcer.hpp"

class AuditReport {
public:
    static bool writeJson(const std::string& path, const std::vector<PolicyRule>& rules);
};

