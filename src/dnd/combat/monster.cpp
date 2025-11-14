#include "dnd/monster.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

namespace dnd {

static void to_json(json& j, const MonsterStats& s) {
    j = json{
        {"str", s.str},
        {"dex", s.dex},
        {"con", s.con},
        {"intl", s.intl},
        {"wis", s.wis},
        {"cha", s.cha}
    };
}

static void from_json(const json& j, MonsterStats& s) {
    s.str  = j.value("str", 10);
    s.dex  = j.value("dex", 10);
    s.con  = j.value("con", 10);
    s.intl = j.value("intl", 10);
    s.wis  = j.value("wis", 10);
    s.cha  = j.value("cha", 10);
}

static void to_json(json& j, const Monster& m) {
    j = json{
        {"id", m.id},
        {"name", m.name},
        {"level", m.level},
        {"hpCurrent", m.hpCurrent},
        {"hpMax", m.hpMax},
        {"ac", m.ac},
        {"stats", m.stats}
    };
}

static void from_json(const json& j, Monster& m) {
    m.id        = j.value("id", std::string{});
    m.name      = j.value("name", std::string{});
    m.level     = j.value("level", 1);
    m.hpCurrent = j.value("hpCurrent", 1);
    m.hpMax     = j.value("hpMax", 1);
    m.ac        = j.value("ac", 10);
    if (j.contains("stats")) {
        m.stats = j.at("stats").get<MonsterStats>();
    }
}

MonsterService::MonsterService(const std::string& storagePath)
    : path_(storagePath) {}

bool MonsterService::load() {
    std::ifstream in(path_);
    if (!in) {
        // kein Fehler, wenn Datei noch nicht existiert
        monsters_.clear();
        return true;
    }
    try {
        json j;
        in >> j;
        monsters_.clear();
        if (j.is_array()) {
            for (auto& e : j) {
                Monster m = e.get<Monster>();
                monsters_.push_back(m);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool MonsterService::save() const {
    try {
        json arr = json::array();
        for (const auto& m : monsters_) {
            json j = m;
            arr.push_back(j);
        }
        std::ofstream out(path_);
        if (!out) return false;
        out << arr.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<Monster> MonsterService::list() const {
    return monsters_;
}

bool MonsterService::get(const std::string& id, Monster& out) const {
    for (const auto& m : monsters_) {
        if (m.id == id) {
            out = m;
            return true;
        }
    }
    return false;
}

bool MonsterService::upsert(const Monster& m) {
    // update falls vorhanden
    for (auto& existing : monsters_) {
        if (existing.id == m.id) {
            existing = m;
            return true;
        }
    }
    // sonst neu
    monsters_.push_back(m);
    return true;
}

bool MonsterService::remove(const std::string& id) {
    auto it = std::remove_if(monsters_.begin(), monsters_.end(),
                             [&](const Monster& m){ return m.id == id; });
    if (it == monsters_.end()) return false;
    monsters_.erase(it, monsters_.end());
    return true;
}

} // namespace dnd

