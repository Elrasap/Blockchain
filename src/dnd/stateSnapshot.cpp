#include "dnd/stateSnapshot.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using nlohmann::json;

namespace dnd {

bool StateSnapshotIO::write(const DndState& st, const std::string& path) {
    json j;

    // Characters
    j["characters"] = json::object();
    for (auto& [id, c] : st.characters)
        j["characters"][id] = c;

    // Monsters
    j["monsters"] = json::object();
    for (auto& [id, m] : st.monsters)
        j["monsters"][id] = m;

    // Encounters
    j["encounters"] = json::object();
    for (auto& [id, e] : st.encounters)
        j["encounters"][id] = e;

    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << j.dump(2);
    return true;
}

bool StateSnapshotIO::load(DndState& st, const std::string& path) {
    try {
        std::ifstream in(path);
        if (!in.is_open()) return false;

        json j;
        in >> j;

        st.clear();

        // load characters
        for (auto& [id, jc] : j["characters"].items()) {
            CharacterState c;
            c = jc.get<CharacterState>();
            st.characters[id] = c;
        }

        // load monsters
        for (auto& [id, jm] : j["monsters"].items()) {
            MonsterState m;
            m = jm.get<MonsterState>();
            st.monsters[id] = m;
        }

        // load encounters
        for (auto& [id, je] : j["encounters"].items()) {
            EncounterState e;
            e = je.get<EncounterState>();
            st.encounters[id] = e;
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

} // namespace dnd

