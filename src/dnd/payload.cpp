#include "dnd/payload.hpp"
#include <stdexcept>

namespace dnd {

using json = nlohmann::json;

void to_json(json& j, const DndPayload& p) {
    j = json{
        {"type", static_cast<int>(p.type)},
        {"jsonData", p.jsonData}
    };
}

void from_json(const json& j, DndPayload& p) {
    int t;
    j.at("type").get_to(t);
    p.type = static_cast<PayloadType>(t);
    j.at("jsonData").get_to(p.jsonData);
}

std::string serializePayload(const DndPayload& p) {
    return json(p).dump();
}

DndPayload deserializePayload(const std::string& s) {
    auto j = json::parse(s);
    return j.get<DndPayload>();
}

} // namespace dnd

