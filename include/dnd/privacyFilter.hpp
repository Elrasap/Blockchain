#pragma once
#include <string>
#include <vector>
#include <memory>

class PrivacyFilter {
public:
    std::string filterResponse(const std::string& json, const std::string& role);
    bool canAccessField(const std::string& field, const std::string& role);


};
