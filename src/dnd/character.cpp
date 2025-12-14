#include "dnd/character.hpp"
#include <stdexcept>

namespace dnd {

using nlohmann::json;

// interne Hilfsfunktionen für Enums <-> String
namespace {

std::string classToString(CharacterClass c) {
    switch (c) {
        case CharacterClass::Fighter:   return "Fighter";
        case CharacterClass::Wizard:    return "Wizard";
        case CharacterClass::Rogue:     return "Rogue";
        case CharacterClass::Cleric:    return "Cleric";
        case CharacterClass::Ranger:    return "Ranger";
        case CharacterClass::Barbarian: return "Barbarian";
        case CharacterClass::Custom:    return "Custom";
    }
    return "Custom";
}

CharacterClass classFromString(const std::string& s) {
    if (s == "Fighter")   return CharacterClass::Fighter;
    if (s == "Wizard")    return CharacterClass::Wizard;
    if (s == "Rogue")     return CharacterClass::Rogue;
    if (s == "Cleric")    return CharacterClass::Cleric;
    if (s == "Ranger")    return CharacterClass::Ranger;
    if (s == "Barbarian") return CharacterClass::Barbarian;
    return CharacterClass::Custom;
}

std::string raceToString(Race r) {
    switch (r) {
        case Race::Human:      return "Human";
        case Race::Elf:        return "Elf";
        case Race::Dwarf:      return "Dwarf";
        case Race::Halfling:   return "Halfling";
        case Race::Orc:        return "Orc";
        case Race::Dragonborn: return "Dragonborn";
        case Race::Custom:     return "Custom";
    }
    return "Custom";
}

Race raceFromString(const std::string& s) {
    if (s == "Human")      return Race::Human;
    if (s == "Elf")        return Race::Elf;
    if (s == "Dwarf")      return Race::Dwarf;
    if (s == "Halfling")   return Race::Halfling;
    if (s == "Orc")        return Race::Orc;
    if (s == "Dragonborn") return Race::Dragonborn;
    return Race::Custom;
}

} // namespace

/* ---------- Default-Character ---------- */

CharacterSheet makeDefaultCharacter(
    const std::string& id,
    const std::string& playerAddress,
    const std::string& name,
    CharacterClass cls,
    Race race
) {
    CharacterSheet c;
    c.id            = id;
    c.playerAddress = playerAddress;
    c.name          = name;
    c.cls           = cls;
    c.race          = race;

    c.level      = 1;
    c.hpMax      = 10;
    c.hpCurrent  = 10;
    c.armorClass = 10;
    c.xp         = 0;

    c.stats = AbilityScores{};
    return c;
}

/* ---------- AbilityScores JSON ---------- */

void to_json(json& j, const AbilityScores& a) {
    j = json{
        {"str", a.str},
        {"dex", a.dex},
        {"con", a.con},
        {"int", a.intl},
        {"wis", a.wis},
        {"cha", a.cha}
    };
}

void from_json(const json& j, AbilityScores& a) {
    j.at("str").get_to(a.str);
    j.at("dex").get_to(a.dex);
    j.at("con").get_to(a.con);
    j.at("int").get_to(a.intl);
    j.at("wis").get_to(a.wis);
    j.at("cha").get_to(a.cha);
}

/* ---------- PlayerAccount JSON ---------- */

void to_json(json& j, const PlayerAccount& p) {
    j = json{
        {"address",         p.address},
        {"publicKeyHex",    p.publicKeyHex},
        {"displayName",     p.displayName},
        {"isDungeonMaster", p.isDungeonMaster},
        {"permissionLevel", p.permissionLevel}
    };
}

void from_json(const json& j, PlayerAccount& p) {
    j.at("address").get_to(p.address);
    j.at("publicKeyHex").get_to(p.publicKeyHex);
    j.at("displayName").get_to(p.displayName);
    j.at("isDungeonMaster").get_to(p.isDungeonMaster);
    j.at("permissionLevel").get_to(p.permissionLevel);
}

/* ---------- CharacterSheet JSON ---------- */

void to_json(json& j, const CharacterSheet& c) {
    j = json{
        {"id",            c.id},
        {"playerAddress", c.playerAddress},
        {"name",          c.name},
        {"class",         classToString(c.cls)},
        {"race",          raceToString(c.race)},
        {"level",         c.level},
        {"hpCurrent",     c.hpCurrent},
        {"hpMax",         c.hpMax},
        {"armorClass",    c.armorClass},
        {"xp",            c.xp},
        {"stats",         c.stats},
        {"inventory",     c.inventory},
        {"notes",         c.notes}
    };
}

void from_json(const json& j, CharacterSheet& c) {
    j.at("id").get_to(c.id);
    j.at("playerAddress").get_to(c.playerAddress);
    j.at("name").get_to(c.name);

    std::string clsStr, raceStr;
    j.at("class").get_to(clsStr);
    j.at("race").get_to(raceStr);
    c.cls  = classFromString(clsStr);
    c.race = raceFromString(raceStr);

    j.at("level").get_to(c.level);
    j.at("hpCurrent").get_to(c.hpCurrent);
    j.at("hpMax").get_to(c.hpMax);

    // armorClass optional, damit alte Saves noch laden
    c.armorClass = j.value("armorClass", 10);

    j.at("xp").get_to(c.xp);
    j.at("stats").get_to(c.stats);
    j.at("inventory").get_to(c.inventory);
    j.at("notes").get_to(c.notes);
}

/* ---------- Convenience Wrappers ---------- */

std::string serializeCharacter(const CharacterSheet& c) {
    json j = c;
    return j.dump();
}

CharacterSheet deserializeCharacter(const std::string& jsonStr) {
    json j = json::parse(jsonStr);
    CharacterSheet c = j.get<CharacterSheet>();
    return c;
}

std::string serializePlayer(const PlayerAccount& p) {
    json j = p;
    return j.dump();
}

PlayerAccount deserializePlayer(const std::string& jsonStr) {
    json j = json::parse(jsonStr);
    PlayerAccount p = j.get<PlayerAccount>();
    return p;
}

} // namespace dnd

