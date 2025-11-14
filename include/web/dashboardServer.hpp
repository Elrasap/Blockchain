#pragma once
#include <string>
#include "dnd/dndCharacterService.hpp"
#include "dnd/combat/combatService.hpp"
#include "dnd/combat/encounter.hpp"
#include "dnd/combat/combatLog.hpp"

class DashboardServer {
public:
    DashboardServer(int p, const std::string& dir, const std::string& bin);

    void start();

private:
    int port;
    std::string reportsDir;
    std::string binaryPath;

    dnd::DndCharacterService dndService;
    dnd::combat::CombatService combatService;
    dnd::combat::Encounter encounter;
    dnd::combat::CombatLog combatLog;

    void logDnDTx(const std::string& j);
};

