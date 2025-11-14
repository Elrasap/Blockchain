#include "dnd/combat/combatLog.hpp"
#include <fstream>

namespace dnd::combat {

CombatLog::CombatLog(const std::string& filePath)
: path(filePath) {}

void CombatLog::addEntry(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mtx);

    buffer.push_back(j);
    if (buffer.size() > 1000) {
        buffer.erase(buffer.begin(), buffer.begin() + (buffer.size() - 1000));
    }

    try {
        std::ofstream out(path, std::ios::app);
        out << j.dump() << "\n";
    } catch (...) {
        // ignore
    }
}

std::vector<nlohmann::json> CombatLog::recent(size_t maxEntries) const {
    std::lock_guard<std::mutex> lock(mtx);

    std::vector<nlohmann::json> out;
    if (buffer.size() <= maxEntries) {
        out = buffer;
    } else {
        out.assign(buffer.end() - maxEntries, buffer.end());
    }
    return out;
}

nlohmann::json CombatLog::toJson(size_t maxEntries) const {
    nlohmann::json j;
    j["entries"] = nlohmann::json::array();
    auto rec = recent(maxEntries);
    for (auto& e : rec) {
        j["entries"].push_back(e);
    }
    return j;
}

} // namespace dnd::combat

