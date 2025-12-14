#pragma once
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace dnd {

static constexpr const char* DND_PREFIX = "DND:";

inline bool isDndPayload(const std::vector<uint8_t>& raw)
{
    if (raw.size() < 4)
        return false;

    std::string prefix(raw.begin(), raw.begin() + 4);

    return prefix == DND_PREFIX;
}

inline std::vector<uint8_t> makeDndPayload(const nlohmann::json& j)
{
    std::string s = std::string(DND_PREFIX) + j.dump();
    return std::vector<uint8_t>(s.begin(), s.end());
}

inline nlohmann::json parseDndPayload(const std::vector<uint8_t>& raw)
{
    std::string s(raw.begin(), raw.end());

    if (s.rfind(DND_PREFIX, 0) != 0)
        throw std::runtime_error("Not a DND payload");

    std::string jsonPart = s.substr(4);
    return nlohmann::json::parse(jsonPart);
}

}

