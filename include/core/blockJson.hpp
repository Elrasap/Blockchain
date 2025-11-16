#pragma once
#include <nlohmann/json.hpp>
#include "core/block.hpp"

// JSON-Serialisierung für BlockHeader und Block.
// Deklaration – Implementierung in src/core/blockJson.cpp

void to_json(nlohmann::json& j, const BlockHeader& h);
void from_json(const nlohmann::json& j, BlockHeader& h);

void to_json(nlohmann::json& j, const Block& b);
void from_json(const nlohmann::json& j, Block& b);

