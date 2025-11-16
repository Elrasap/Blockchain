#pragma once
#include <vector>
#include <cstdint>
#include "core/block.hpp"
#include "storage/blockStore.hpp"

// Hohe Ebene "sichtbare" Blockchain, die BlockStore + PoA-Validierung kapselt.
class Blockchain {
public:
    // dmValidatorPubKey = Public Key des Validators (DM)
    Blockchain(BlockStore& store,
               const std::vector<uint8_t>& dmValidatorPubKey);

    // Aktuelle Höhe der Chain (höchster Block).
    uint64_t getHeight() const;

    // Letzter Block in der Chain. Bei leerer Chain → Default-Block mit height=0.
    Block getLatestBlock() const;

    // Block mit bestimmter Höhe. Wenn nicht gefunden → Default-Block (height=0).
    Block getBlock(uint64_t height) const;

    // Lädt die gesamte Chain (alle Blöcke) aus dem Store.
    std::vector<Block> loadChain() const;

    // Validiert einen Block (inkl. Proof-of-Authority gegen dmPubKey).
    bool validateBlock(const Block& block) const;

    // Hängt einen Block an die Chain an (nach Validierung).
    // Erwartung: Block ist für die Spitze (prev = aktueller letzter Block).
    bool appendBlock(const Block& block);

    // Erstellt einen Genesis-Block (height=0), falls DB leer ist.
    // Nutzt dmPrivKey, um den Genesis-Header zu signieren.
    bool ensureGenesisBlock(const std::vector<uint8_t>& dmPrivKey);

private:
    BlockStore& store;
    std::vector<uint8_t> dmPubKey;
};

