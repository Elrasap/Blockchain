#pragma once
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace dnd {

// Patch-Objekt: beliebige Felder -> neue Werte (als JSON)
struct CharacterPatch {
    std::string characterId;
    nlohmann::json patchData; // {"hpCurrent": 4, "xp": 120, ...}
};

void to_json(nlohmann::json& j, const CharacterPatch& p);
void from_json(const nlohmann::json& j, CharacterPatch& p);

}

