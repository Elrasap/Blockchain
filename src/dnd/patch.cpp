#include "dnd/patch.hpp"

namespace dnd {

using json = nlohmann::json;

void to_json(json& j, const CharacterPatch& p) {
    j = json{
        {"characterId", p.characterId},
        {"patchData", p.patchData}
    };
}

void from_json(const json& j, CharacterPatch& p) {
    j.at("characterId").get_to(p.characterId);
    j.at("patchData").get_to(p.patchData);
}

}

