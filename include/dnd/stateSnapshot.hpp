#pragma once
#include <string>

namespace dnd {

class DndState;

bool writeSnapshot(const DndState& state, const std::string& path);

bool loadSnapshot(DndState& state, const std::string& path);

}

