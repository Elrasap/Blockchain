#include "dnd/combat/dice.hpp"
#include <random>
#include <stdexcept>
#include <sstream>
#include <cctype>

namespace dnd::combat {

// ----------------------------------------------------------
// Constructor & Reseeding
// ----------------------------------------------------------

Dice::Dice()
    : rng(std::random_device{}())
{
}

Dice::Dice(uint64_t seed)
    : rng(seed)
{
}

void Dice::reseed(uint64_t seed) {
    rng.seed(seed);
}

// ----------------------------------------------------------
// Helper
// ----------------------------------------------------------

int Dice::rollDie(int sides) {
    std::uniform_int_distribution<int> dist(1, sides);
    return dist(rng);
}

// ----------------------------------------------------------
// roll(count, sides, modifier)
// ----------------------------------------------------------

DiceRoll Dice::roll(int count, int sides, int modifier) {
    if (count <= 0 || sides <= 0) {
        throw std::invalid_argument("Dice::roll: count and sides must be > 0");
    }

    DiceRoll out{};
    out.raw = 0;
    out.modifier = modifier;

    for (int i = 0; i < count; ++i) {
        out.raw += rollDie(sides);
    }

    out.total = out.raw + modifier;
    return out;
}

// ----------------------------------------------------------
// Expression parser: "XdY+Z" oder "d20-1"
// ----------------------------------------------------------

DiceRoll Dice::rollExpr(const std::string& expr) {
    int count = 1;
    int sides = 0;
    int modifier = 0;

    // Beispiel-Patterns:
    //  "1d20"
    //  "d20"
    //  "2d6+3"
    //  "3d8-1"

    size_t dpos = expr.find('d');
    if (dpos == std::string::npos)
        throw std::invalid_argument("Dice::rollExpr: invalid expression: " + expr);

    // vor dem 'd'
    if (dpos == 0)
        count = 1;
    else
        count = std::stoi(expr.substr(0, dpos));

    // rest nach 'd'
    std::string right = expr.substr(dpos + 1);

    // suche nach + oder -
    size_t plus  = right.find('+');
    size_t minus = right.find('-');

    if (plus != std::string::npos) {
        sides = std::stoi(right.substr(0, plus));
        modifier = std::stoi(right.substr(plus + 1));
    }
    else if (minus != std::string::npos) {
        sides = std::stoi(right.substr(0, minus));
        modifier = -std::stoi(right.substr(minus + 1));
    }
    else {
        sides = std::stoi(right);
    }

    return roll(count, sides, modifier);
}

// ----------------------------------------------------------
// parseAndRoll() → einfach Alias auf rollExpr()
// ----------------------------------------------------------

DiceRoll Dice::parseAndRoll(const std::string& expr) {
    return rollExpr(expr);
}

// ----------------------------------------------------------
// roll("1d20") → nur total zurückgeben
// ----------------------------------------------------------

int Dice::roll(const std::string& expr) {
    return rollExpr(expr).total;
}

// ----------------------------------------------------------
// rollD20() mit Advantage/Disadvantage
// ----------------------------------------------------------

D20Roll Dice::rollD20(int modifier, AdvantageState adv) {
    int r1 = rollDie(20);
    int r2 = rollDie(20);

    D20Roll out{};
    out.modifier = modifier;
    out.advantage = adv;

    switch (adv) {
        case AdvantageState::Normal:
            out.natural = r1;
            out.total = r1 + modifier;
            break;

        case AdvantageState::Advantage:
            out.natural = std::max(r1, r2);
            out.altRoll = std::min(r1, r2);
            out.total = out.natural + modifier;
            break;

        case AdvantageState::Disadvantage:
            out.natural = std::min(r1, r2);
            out.altRoll = std::max(r1, r2);
            out.total = out.natural + modifier;
            break;
    }

    return out;
}

// ----------------------------------------------------------
// JSON
// ----------------------------------------------------------

void to_json(nlohmann::json& j, const DiceRoll& r) {
    j = {
        {"total", r.total},
        {"raw", r.raw},
        {"modifier", r.modifier}
    };
}

void from_json(const nlohmann::json& j, DiceRoll& r) {
    j.at("total").get_to(r.total);
    j.at("raw").get_to(r.raw);
    j.at("modifier").get_to(r.modifier);
}

void to_json(nlohmann::json& j, const D20Roll& r) {
    j = {
        {"total", r.total},
        {"natural", r.natural},
        {"modifier", r.modifier},
        {"advantage", static_cast<int>(r.advantage)},
        {"altRoll", r.altRoll}
    };
}

void from_json(const nlohmann::json& j, D20Roll& r) {
    j.at("total").get_to(r.total);
    j.at("natural").get_to(r.natural);
    j.at("modifier").get_to(r.modifier);

    int a = 0;
    j.at("advantage").get_to(a);
    r.advantage = static_cast<AdvantageState>(a);

    j.at("altRoll").get_to(r.altRoll);
}

} // namespace dnd::combat

