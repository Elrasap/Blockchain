#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

namespace dnd::combat {

class CombatLog {
public:
    explicit CombatLog(const std::string& filePath = "combat.log");

    void addEntry(const nlohmann::json& j);
    std::vector<nlohmann::json> recent(size_t maxEntries) const;
    nlohmann::json toJson(size_t maxEntries) const;

private:
    std::string path;
    mutable std::mutex mtx;
    std::vector<nlohmann::json> buffer;   // In-Memory Ring-Buffer
};

} // namespace dnd::combat

