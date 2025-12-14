#pragma once
#include <string>
#include <vector>
#include "incident/incident.hpp"

class ForensicsReporter {
public:
    ForensicsReporter();
    void writeReport(const Incident& inc, const std::string& dir);
private:
    std::string hashString(const std::string& input);
};

