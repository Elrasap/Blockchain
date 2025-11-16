#pragma once
#include <string>
#include "dnd/state.hpp"

namespace dnd {

class DndState;

struct StateSnapshotIO {
    static bool write(const DndState& st, const std::string& path);
    static bool load(DndState& st, const std::string& path);
};

} // namespace dnd

