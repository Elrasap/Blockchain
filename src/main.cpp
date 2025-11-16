#include "core/dmKeyManager.hpp"
#include "storage/blockStore.hpp"
#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"
#include <iostream>
#include <memory>

int main() {
    // 1. DM-Key laden/erzeugen
    DmKeyPair dmKeys;
    if (!loadOrCreateDmKey("dm_keys.bin", dmKeys)) {
        std::cerr << "Failed to init DM keys\n";
        return 1;
    }

    // 2. BlockStore
    BlockStore store("blocks.db");

    // 3. Blockchain
    Blockchain chain(store, dmKeys.publicKey);

    // 4. Genesis sicherstellen
    chain.ensureGenesisBlock(dmKeys.privateKey);

    // 5. BlockBuilder
    BlockBuilder builder(chain, dmKeys.privateKey, dmKeys.publicKey);

    // 6. Mempool
    Mempool mempool;
    // ... mempool mit Transaktionen füllen ...

    auto txs = mempool.getAll();

    if (!txs.empty()) {
        Block newBlock = builder.buildBlock(txs);

        if (chain.appendBlock(newBlock)) {
            mempool.clear();
            std::cout << "New block appended.\n";
        }
    }

    return 0;
}

