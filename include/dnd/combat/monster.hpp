#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace dnd {

struct MonsterStats {
    int str  = 10;
    int dex  = 10;
    int con  = 10;
    int intl = 10;
    int wis  = 10;
    int cha  = 10;
};

struct Monster {
    std::string id;
    std::string name;
    int level     = 1;
    int hpCurrent = 1;
    int hpMax     = 1;
    int ac        = 10;
    MonsterStats stats;
};

class MonsterService {
public:
    explicit MonsterService(const std::string& storagePath = "monsters.json");

    bool load();
    bool save() const;

    std::vector<Monster> list() const;
    bool get(const std::string& id, Monster& out) const;
    bool upsert(const Monster& m);        // create or update
    bool remove(const std::string& id);

private:
    std::string path_;
    std::vector<Monster> monsters_;
};

// JSON helpers
inline void to_json(nlohmann::json& j, const MonsterStats& s) {
    j = nlohmann::json{
        {"str",  s.str},
        {"dex",  s.dex},
        {"con",  s.con},
        {"int",  s.intl},
        {"wis",  s.wis},
        {"cha",  s.cha}
    };
}

inline void from_json(const nlohmann::json& j, MonsterStats& s) {
    s.str  = j.value("str",  10);
    s.dex  = j.value("dex",  10);
    s.con  = j.value("con",  10);
    s.intl = j.value("int",  10);
    s.wis  = j.value("wis",  10);
    s.cha  = j.value("cha",  10);
}

inline void to_json(nlohmann::json& j, const Monster& m) {
    j = nlohmann::json{
        {"id",        m.id},
        {"name",      m.name},
        {"level",     m.level},
        {"hpCurrent", m.hpCurrent},
        {"hpMax",     m.hpMax},
        {"ac",        m.ac},
        {"stats",     m.stats}
    };
}

inline void from_json(const nlohmann::json& j, Monster& m) {
    m.id        = j.value("id",   std::string{});
    m.name      = j.value("name", std::string{});
    m.level     = j.value("level", 1);
    m.hpCurrent = j.value("hpCurrent", 1);
    m.hpMax     = j.value("hpMax", 1);
    m.ac        = j.value("ac", 10);
    if (j.contains("stats")) {
        m.stats = j.at("stats").get<MonsterStats>();
    }
}

} // namespace dnd

