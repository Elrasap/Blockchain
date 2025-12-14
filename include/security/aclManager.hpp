#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class ACLManager {
public:
    bool hasPermission(const std::string& role, const std::string& action);
    void assignRole(const std::string& address, const std::string& role);
    void removeRole(const std::string& address);
    std::string getRole(const std::string& address);
};
