#include "dnd/combat/monster.hpp"
#include <fstream>
#include <algorithm>

namespace dnd {

MonsterService::MonsterService(const std::string& storagePath)
    : path_(storagePath)
{
}

bool MonsterService::load() {
    std::ifstream in(path_);
    if (!in) return true; // kein File = ok

    try {
        nlohmann::json j;
        in >> j;
        monsters_ = j.get<std::vector<Monster>>();
        return true;
    } catch (...) {
        return false;
    }
}

bool MonsterService::save() const {
    try {
        nlohmann::json j = monsters_;
        std::ofstream out(path_);
        out << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<Monster> MonsterService::list() const {
    return monsters_;
}

bool MonsterService::get(const std::string& id, Monster& out) const {
    for (auto& m : monsters_) {
        if (m.id == id) {
            out = m;
            return true;
        }
    }
    return false;
}

bool MonsterService::upsert(const Monster& m) {
    for (auto& x : monsters_) {
        if (x.id == m.id) {
            x = m;
            return true;
        }
    }
    monsters_.push_back(m);
    return true;
}

bool MonsterService::remove(const std::string& id) {
    auto oldSize = monsters_.size();
    monsters_.erase(
        std::remove_if(monsters_.begin(), monsters_.end(),
                       [&](const Monster& m){ return m.id == id; }),
        monsters_.end());

    return monsters_.size() != oldSize;
}

} // namespace dnd

