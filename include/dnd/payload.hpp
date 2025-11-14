#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace dnd {

enum class PayloadType : uint8_t {
    DND_CREATE_CHARACTER = 1,
    DND_UPDATE_CHARACTER = 2
};

struct DndPayload {
    PayloadType type;
    std::string jsonData; // enthält CharacterSheet oder Patch
};

// JSON serialization
void to_json(nlohmann::json& j, const DndPayload& p);
void from_json(const nlohmann::json& j, DndPayload& p);

std::string serializePayload(const DndPayload& p);
DndPayload deserializePayload(const std::string& s);

} // namespace dnd

