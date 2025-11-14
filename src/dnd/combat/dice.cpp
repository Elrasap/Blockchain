#include "dnd/combat/dice.hpp"
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace dnd::combat {

Dice::Dice()
: rng(std::random_device{}()) {}

Dice::Dice(uint64_t seed)
: rng(seed) {}

void Dice::reseed(uint64_t seed) {
    rng.seed(seed);
}

int Dice::rollDie(int sides) {
    std::uniform_int_distribution<int> dist(1, sides);
    return dist(rng);
}

DiceRoll Dice::roll(int count, int sides, int modifier) {
    DiceRoll r;
    r.expr = std::to_string(count) + "d" + std::to_string(sides);
    r.modifier = modifier;

    for (int i = 0; i < count; ++i) {
        int v = rollDie(sides);
        r.rolls.push_back(v);
        r.total += v;
    }
    r.total += modifier;
    return r;
}

DiceRoll Dice::parseAndRoll(const std::string& expr) {
    // sehr simple Parser-Logik:
    //   [count] 'd' [sides] [+-modifier]
    //   Beispiele: "d20+5", "2d6-1", "1d8", "3d4+0"
    int count = 1;
    int sides = 20;
    int modifier = 0;

    std::string s = expr;
    for (auto& c : s) c = static_cast<char>(std::tolower(c));

    // finde d
    auto posD = s.find('d');
    if (posD == std::string::npos) {
        throw std::runtime_error("Invalid dice expr: " + expr);
    }

    // vor d: count
    if (posD == 0) {
        count = 1;
    } else {
        count = std::stoi(s.substr(0, posD));
    }

    // nach d: sides (+ optional + oder -)
    int posSign = -1;
    for (size_t i = posD + 1; i < s.size(); ++i) {
        if (s[i] == '+' || s[i] == '-') {
            posSign = static_cast<int>(i);
            break;
        }
    }

    if (posSign == -1) {
        sides = std::stoi(s.substr(posD + 1));
        modifier = 0;
    } else {
        sides = std::stoi(s.substr(posD + 1, posSign - (posD + 1)));
        modifier = std::stoi(s.substr(posSign));
    }

    return roll(count, sides, modifier);
}

DiceRoll Dice::rollExpr(const std::string& expr) {
    DiceRoll r = parseAndRoll(expr);
    r.expr = expr;
    return r;
}

D20Roll Dice::rollD20(int modifier, AdvantageState adv) {
    D20Roll res;
    res.modifier = modifier;
    res.advantage = adv;

    int r1 = rollDie(20);
    int r2 = 0;
    int nat = r1;
    int total = r1 + modifier;

    if (adv == AdvantageState::Advantage || adv == AdvantageState::Disadvantage) {
        r2 = rollDie(20);
        res.altRoll = r2;

        if (adv == AdvantageState::Advantage) {
            nat = std::max(r1, r2);
        } else {
            nat = std::min(r1, r2);
        }
        total = nat + modifier;
    }

    res.natural = nat;
    res.total = total;
    return res;
}

/* ------------- JSON für DiceRoll ------------- */

void to_json(nlohmann::json& j, const DiceRoll& r) {
    j = nlohmann::json{
        {"total", r.total},
        {"modifier", r.modifier},
        {"rolls", r.rolls},
        {"expr", r.expr}
    };
}

void from_json(nlohmann::json& j, DiceRoll& r) {
    j.at("total").get_to(r.total);
    j.at("modifier").get_to(r.modifier);
    j.at("rolls").get_to(r.rolls);
    if (j.contains("expr")) {
        j.at("expr").get_to(r.expr);
    } else {
        r.expr = "";
    }
}

/* ------------- JSON für D20Roll ------------- */

void to_json(nlohmann::json& j, const D20Roll& r) {
    j = nlohmann::json{
        {"total", r.total},
        {"natural", r.natural},
        {"modifier", r.modifier},
        {"advantage", advantageToString(r.advantage)},
        {"altRoll", r.altRoll}
    };
}

void from_json(nlohmann::json& j, D20Roll& r) {
    j.at("total").get_to(r.total);
    j.at("natural").get_to(r.natural);
    j.at("modifier").get_to(r.modifier);
    if (j.contains("altRoll")) {
        j.at("altRoll").get_to(r.altRoll);
    } else {
        r.altRoll = 0;
    }
    if (j.contains("advantage")) {
        r.advantage = advantageFromString(j.at("advantage").get<std::string>());
    } else {
        r.advantage = AdvantageState::Normal;
    }
}

} // namespace dnd::combat

