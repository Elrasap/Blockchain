#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace dnd {

/* ===========================
   ENUMS: CLASS & RACE
   =========================== */

enum class CharacterClass {
    Fighter,
    Wizard,
    Rogue,
    Cleric,
    Ranger,
    Barbarian,
    Custom
};

enum class Race {
    Human,
    Elf,
    Dwarf,
    Halfling,
    Orc,
    Dragonborn,
    Custom
};

/* ===========================
   ABILITY SCORES
   =========================== */

struct AbilityScores {
    int str  = 10;
    int dex  = 10;
    int con  = 10;
    int intl = 10;
    int wis  = 10;
    int cha  = 10;
};

// Kompatibilität: alter Name "Stats"
using Stats = AbilityScores;

/* ===========================
   PLAYER ACCOUNT
   =========================== */

struct PlayerAccount {
    std::string address;        // Wallet / Chain-Adresse
    std::string publicKeyHex;   // Public Key in Hex
    std::string displayName;    // Anzeigename im UI
    bool        isDungeonMaster = false;
    int         permissionLevel  = 0;
};

/* ===========================
   CHARACTER SHEET
   =========================== */

struct CharacterSheet {
    std::string id;
    std::string playerAddress;   // Referenz auf PlayerAccount.address
    std::string name;

    CharacterClass cls = CharacterClass::Custom;
    Race           race = Race::Custom;

    int level      = 1;

    int hpCurrent  = 10;
    int hpMax      = 10;

    int armorClass = 10;         // für Combat / Trefferwürfe

    int xp         = 0;

    AbilityScores stats;
    std::vector<std::string> inventory;
    std::string notes;
};

/* ===========================
   JSON-FUNKTIONEN (DEKLARATION)
   =========================== */

using nlohmann::json;

void to_json(json& j, const AbilityScores& a);
void from_json(const json& j, AbilityScores& a);

void to_json(json& j, const PlayerAccount& p);
void from_json(const json& j, PlayerAccount& p);

void to_json(json& j, const CharacterSheet& c);
void from_json(const json& j, CharacterSheet& c);

/* ===========================
   HELFER & SERIALISIERUNG
   =========================== */

CharacterSheet makeDefaultCharacter(
    const std::string& id,
    const std::string& playerAddress,
    const std::string& name,
    CharacterClass cls,
    Race race
);

std::string serializeCharacter(const CharacterSheet& c);
CharacterSheet deserializeCharacter(const std::string& jsonStr);

std::string serializePlayer(const PlayerAccount& p);
PlayerAccount deserializePlayer(const std::string& jsonStr);

} // namespace dnd

