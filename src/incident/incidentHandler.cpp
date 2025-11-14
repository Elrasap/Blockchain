#include "incident/incidentHandler.hpp"
#include <iostream>

IncidentHandler::IncidentHandler(const std::string& reportDir)
    : dir(reportDir) {}

void IncidentHandler::registerIncident(const Incident& i) {
    queue.push_back(i);
}

void IncidentHandler::processAll() {
    for (auto& i : queue) {
        std::cout << "[IncidentHandler] Processing incident: "
                  << i.type << " (" << i.severity << ")\n";

        reporter.writeReport(i, dir);

        if (i.type == "NODE_FAIL" && i.severity == "CRITICAL")
            std::cout << "[Recovery] Restarting failed node...\n";
        else if (i.type == "SECURITY")
            std::cout << "[Recovery] Rotating keys and isolating peer...\n";
        else if (i.type == "PERFORMANCE")
            std::cout << "[Recovery] Throttling load and collecting metrics...\n";
    }
    queue.clear();
}

