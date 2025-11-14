#pragma once
#include <string>
#include <vector>

namespace dnd {

struct MonsterStats {
    int str = 10;
    int dex = 10;
    int con = 10;
    int intl = 10;
    int wis = 10;
    int cha = 10;
};

struct Monster {
    std::string id;
    std::string name;
    int level = 1;
    int hpCurrent = 1;
    int hpMax = 1;
    int ac = 10;
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

} // namespace dnd

