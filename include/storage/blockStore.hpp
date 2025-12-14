#pragma once
#include <string>
#include <vector>
#include "core/block.hpp"

// BlockStore speichert Blöcke als JSON-Dateien:
//
//   blocks/
//     000000.json   (Genesis)
//     000001.json
//     000002.json
//
// Der Konstruktor-Parameter ist das Verzeichnis (z.B. "blocks").

class BlockStore {
public:
    explicit BlockStore(const std::string& directory);

    // Block anfügen (blocks/00000N.json)
    bool appendBlock(const Block& block);

    // Alias, falls irgendwo append(...) verwendet wird
    bool append(const Block& block) { return appendBlock(block); }

    // Alle Blöcke laden (sortiert nach height)
    std::vector<Block> loadAllBlocks() const;

    // Letzter Block (oder Default-Block, wenn leer)
    Block getLatestBlock() const;

    // Alle Dateien im Verzeichnis löschen
    void clear();

    // Alias für clear()
    void reset() { clear(); }

private:
    std::string directory_;

    std::string filenameForHeight(uint64_t height) const;
};

