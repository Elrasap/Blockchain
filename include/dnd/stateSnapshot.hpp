#pragma once
#include <string>

namespace dnd {

class DndState;

/// Save a full DND state snapshot to JSON file
bool writeSnapshot(const DndState& state, const std::string& path);

/// Load snapshot JSON and overwrite full game state
bool loadSnapshot(DndState& state, const std::string& path);

} // namespace dnd

