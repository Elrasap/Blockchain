#pragma once
#include <vector>
#include <string>
#include <random>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "dnd/combat/payloads.hpp"

namespace dnd::combat {

struct DiceRoll {
    int total;
    int raw;
    int modifier;
};


// Ergebnis eines W20-Rolls mit Advantage/Disadvantage
struct D20Roll {
    int total = 0;
    int natural = 0;
    int modifier = 0;
    AdvantageState advantage = AdvantageState::Normal;
    int altRoll = 0;          // bei Advantage/Disadvantage zweiter Wurf
};

class Dice {
public:
    Dice();
    explicit Dice(uint64_t seed);

    void reseed(uint64_t seed);

    // Genereller Ausdruck: "XdY+Z", "d20+5", "2d6-1" etc.
    DiceRoll rollExpr(const std::string& expr);

    // W20 mit Advantage/Disadvantage
    D20Roll rollD20(int modifier = 0, AdvantageState adv = AdvantageState::Normal);

    // Helper: einfacher Wurf XdY ohne +/-
    DiceRoll roll(int count, int sides, int modifier = 0);
    int roll(const std::string& expr);

private:
    std::mt19937 rng;

    int rollDie(int sides);
    DiceRoll parseAndRoll(const std::string& expr);
};

void to_json(nlohmann::json& j, const DiceRoll& r);
void from_json(const nlohmann::json& j, DiceRoll& r);

void to_json(nlohmann::json& j, const D20Roll& r);
void from_json(const nlohmann::json& j, D20Roll& r);

} // namespace dnd::combat

