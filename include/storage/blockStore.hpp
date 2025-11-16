#pragma once
#include <string>
#include <vector>
#include "core/block.hpp"

// Vorwärtsdeklaration, um sqlite3.h nur im .cpp zu includen
struct sqlite3;

class BlockStore {
public:
    // path = Pfad zur SQLite-DB (z.B. "blocks.db")
    explicit BlockStore(const std::string& path);
    ~BlockStore();

    // Hängt einen Block an (INSERT/REPLACE in SQLite).
    bool appendBlock(const Block& block);

    // Lädt alle Blöcke sortiert nach Höhe (ascending).
    std::vector<Block> loadAllBlocks() const;

    // Liefert den letzten Block; bei leerer Chain: Default-Block (height=0).
    Block getLatestBlock() const;

    // Entfernt alle Einträge, lässt DB-Datei aber bestehen.
    void clear();

    // DB-Datei komplett löschen & Schema neu anlegen.
    void reset();

private:
    bool initSchema();

    std::string db_path;
    sqlite3* db = nullptr;
};

