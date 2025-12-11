#pragma once
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace dnd {

struct CharacterPatch {
    std::string characterId;
    nlohmann::json patchData;
};

void to_json(nlohmann::json& j, const CharacterPatch& p);
void from_json(const nlohmann::json& j, CharacterPatch& p);

}

