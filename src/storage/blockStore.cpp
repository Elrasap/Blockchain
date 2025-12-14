#include "storage/blockStore.hpp"
#include "core/blockJson.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

// -------------------------------------------------------
// Konstruktor
// -------------------------------------------------------
BlockStore::BlockStore(const std::string& directory)
    : directory_(directory)
{
    if (!fs::exists(directory_)) {
        fs::create_directories(directory_);
    }
}

// -------------------------------------------------------
// Helper: Dateiname für Block-Höhe
// -------------------------------------------------------
std::string BlockStore::filenameForHeight(uint64_t height) const {
    std::ostringstream ss;
    ss << directory_ << "/"
       << std::setw(6) << std::setfill('0') << height
       << ".json";
    return ss.str();
}

// -------------------------------------------------------
// Block anfügen
// -------------------------------------------------------
bool BlockStore::appendBlock(const Block& block) {
    try {
        const std::string path = filenameForHeight(block.header.height);
        nlohmann::json j = block;

        std::ofstream out(path);
        if (!out.is_open()) {
            std::cerr << "[BlockStore] Failed to open file for write: " << path << "\n";
            return false;
        }

        out << j.dump(2);
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[BlockStore] appendBlock exception: " << ex.what() << "\n";
        return false;
    }
}

// -------------------------------------------------------
// Alle Blöcke laden
// -------------------------------------------------------
std::vector<Block> BlockStore::loadAllBlocks() const {
    std::vector<Block> blocks;

    if (!fs::exists(directory_)) {
        return blocks;
    }

    for (auto& entry : fs::directory_iterator(directory_)) {
        if (!entry.is_regular_file()) continue;

        auto path = entry.path();
        if (path.extension() != ".json") continue;

        try {
            std::ifstream in(path);
            if (!in.is_open()) {
                std::cerr << "[BlockStore] Failed to open: " << path << "\n";
                continue;
            }

            nlohmann::json j;
            in >> j;

            Block b = j.get<Block>();
            blocks.push_back(std::move(b));
        } catch (const std::exception& ex) {
            std::cerr << "[BlockStore] Error reading " << path
                      << ": " << ex.what() << "\n";
        }
    }

    // Nach height sortieren
    std::sort(blocks.begin(), blocks.end(),
              [](const Block& a, const Block& b) {
                  return a.header.height < b.header.height;
              });

    return blocks;
}

// -------------------------------------------------------
// Letzter Block
// -------------------------------------------------------
Block BlockStore::getLatestBlock() const {
    auto blocks = loadAllBlocks();
    if (blocks.empty()) {
        return Block{};
    }
    return blocks.back();
}

// -------------------------------------------------------
// Alle Block-Dateien löschen
// -------------------------------------------------------
void BlockStore::clear() {
    if (!fs::exists(directory_)) return;

    for (auto& entry : fs::directory_iterator(directory_)) {
        if (entry.is_regular_file()) {
            try {
                fs::remove(entry.path());
            } catch (const std::exception& ex) {
                std::cerr << "[BlockStore] Failed to remove "
                          << entry.path() << ": " << ex.what() << "\n";
            }
        }
    }
}

