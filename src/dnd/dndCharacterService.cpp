#include "dnd/dndCharacterService.hpp"
#include <algorithm>

namespace dnd {

using json = nlohmann::json;

void DndCharacterService::handleCreate(const std::string& data) {
    CharacterSheet c = deserializeCharacter(data);
    characters[c.id] = c;
    history[c.id].push_back(json{
        {"event", "create"},
        {"fullState", json(c)}
    });
}

void DndCharacterService::handleUpdate(const std::string& data) {
    CharacterPatch patch = json::parse(data).get<CharacterPatch>();
    auto it = characters.find(patch.characterId);
    if (it == characters.end()) return;
    CharacterSheet& c = it->second;
    for (auto& kv : patch.patchData.items()) {
        const std::string& key = kv.key();
        const json& value = kv.value();
        if (key == "name") c.name = value.get<std::string>();
        else if (key == "level") c.level = value.get<int>();
        else if (key == "hpCurrent") c.hpCurrent = value.get<int>();
        else if (key == "hpMax") c.hpMax = value.get<int>();
        else if (key == "xp") c.xp = value.get<int>();
        else if (key == "notes") c.notes = value.get<std::string>();
        else if (key == "stats") {
            if (value.contains("str")) c.stats.str = value["str"].get<int>();
            if (value.contains("dex")) c.stats.dex = value["dex"].get<int>();
            if (value.contains("con")) c.stats.con = value["con"].get<int>();
            if (value.contains("int")) c.stats.intl = value["int"].get<int>();
            if (value.contains("wis")) c.stats.wis = value["wis"].get<int>();
            if (value.contains("cha")) c.stats.cha = value["cha"].get<int>();
        } else if (key == "addItem") {
            c.inventory.push_back(value.get<std::string>());
        } else if (key == "removeItem") {
            std::string item = value.get<std::string>();
            auto& inv = c.inventory;
            inv.erase(std::remove(inv.begin(), inv.end(), item), inv.end());
        }
    }
    history[patch.characterId].push_back(json{
        {"event", "update"},
        {"patch", patch.patchData}
    });
}

void DndCharacterService::applyCreateJson(const std::string& jsonData) {
    handleCreate(jsonData);
}

void DndCharacterService::applyUpdateJson(const std::string& jsonData) {
    handleUpdate(jsonData);
}

std::vector<CharacterSheet> DndCharacterService::listCharacters() const {
    std::vector<CharacterSheet> out;
    out.reserve(characters.size());
    for (auto& kv : characters) {
        out.push_back(kv.second);
    }
    return out;
}

bool DndCharacterService::getCharacter(const std::string& id, CharacterSheet& out) const {
    auto it = characters.find(id);
    if (it == characters.end()) return false;
    out = it->second;
    return true;
}

}

