#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace dnd {

enum class CharacterClass : std::uint8_t {
    Fighter,
    Wizard,
    Rogue,
    Cleric,
    Ranger,
    Barbarian,
    Custom
};

enum class Race : std::uint8_t {
    Human,
    Elf,
    Dwarf,
    Halfling,
    Orc,
    Dragonborn,
    Custom
};

struct AbilityScores {
    int str{10};
    int dex{10};
    int con{10};
    int intl{10};
    int wis{10};
    int cha{10};
};

struct PlayerAccount {
    // z.B. Hash des Public Keys
    std::string address;
    // hex-codierter öffentlicher Schlüssel (Ed25519 o.ä.)
    std::string publicKeyHex;
    // Anzeigename (Spielername / Discord-Name, etc.)
    std::string displayName;
    // ist das der Dungeon Master?
    bool isDungeonMaster{false};
    // z.B. 0 = Spieler, 10 = Co-DM, 100 = DM
    std::uint32_t permissionLevel{0};
};

struct CharacterSheet {
    std::string id;              // eindeutige ID (z.B. UUID oder address+name)
    std::string playerAddress;   // gehört zu welchem PlayerAccount
    std::string name;
    CharacterClass cls{CharacterClass::Custom};
    Race race{Race::Custom};
    int level{1};
    int hpCurrent{10};
    int hpMax{10};
    int xp{0};
    AbilityScores stats;
    std::vector<std::string> inventory;
    std::string notes;
};

// Hilfsfunktion: erstellt ein Standard-Char-Template
CharacterSheet makeDefaultCharacter(
    const std::string& id,
    const std::string& playerAddress,
    const std::string& name,
    CharacterClass cls,
    Race race
);

// JSON-Serialisierung
void to_json(nlohmann::json& j, const AbilityScores& a);
void from_json(const nlohmann::json& j, AbilityScores& a);

void to_json(nlohmann::json& j, const PlayerAccount& p);
void from_json(const nlohmann::json& j, PlayerAccount& p);

void to_json(nlohmann::json& j, const CharacterSheet& c);
void from_json(const nlohmann::json& j, CharacterSheet& c);

// Komfortfunktionen für Payloads (z.B. in Transactions)
std::string serializeCharacter(const CharacterSheet& c);
CharacterSheet deserializeCharacter(const std::string& jsonStr);

std::string serializePlayer(const PlayerAccount& p);
PlayerAccount deserializePlayer(const std::string& jsonStr);

} // namespace dnd

