#pragma once
#include "incident/incident.hpp"
#include "incident/forensicsReporter.hpp"
#include <vector>

class IncidentHandler {
public:
    explicit IncidentHandler(const std::string& reportDir);
    void registerIncident(const Incident& i);
    void processAll();
private:
    std::vector<Incident> queue;
    std::string dir;
    ForensicsReporter reporter;
};

