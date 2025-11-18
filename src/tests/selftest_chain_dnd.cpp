#include "tests/testFramework.hpp"
#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"
#include "core/transaction.hpp"
#include "core/crypto.hpp"
#include "storage/blockStore.hpp"
#include "core/state.hpp"

#include "dnd/dndTx.hpp"
#include "dnd/dndTxSerialization.hpp"
#include "dnd/dndTxAdapter.hpp"   // <- wichtig: bringt wrapDndTxIntoTransaction()

#include <cstdio>
#include <ctime>

using dnd::DndEventTx;
using dnd::DndState;

/**
 * Helper: erzeugt ein einfaches "Hit"-DnD-Event.
 */
static DndEventTx makeSimpleHitEvt() {
    DndEventTx evt{};
    evt.encounterId = "enc-1";
    evt.actorId     = "hero-1";
    evt.actorType   = 0;            // Character
    evt.targetId    = "goblin-1";
    evt.targetType  = 1;            // Monster
    evt.hit         = true;
    evt.damage      = 5;
    evt.timestamp   = time(nullptr);
    return evt;
}

TEST_CASE(selftest_chain_with_single_dnd_tx) {
    //
    // 1. Temp-Blockchain-DB vorbereiten
    //
    const char* dbPath = "selftest_blocks.db";
    std::remove(dbPath);
    BlockStore store(dbPath);

    //
    // 2. Dungeon-Master-Key (Proof-of-Authority)
    //
    auto dmKeys = crypto::generateKeyPair();

    Blockchain chain(store, dmKeys.publicKey);
    chain.ensureGenesisBlock(dmKeys.privateKey);

    //
    // 3. Mempool + BlockBuilder
    //
    Mempool mempool;  // aktuelle, einfache Mempool-Version
    BlockBuilder builder(chain, dmKeys.privateKey, dmKeys.publicKey);

    //
    // 4. Spieler-Key erzeugen
    //
    std::vector<uint8_t> playerPub, playerPriv;
    dnd::generatePlayerKeypair(playerPub, playerPriv);

    //
    // 5. DnD-Event erzeugen und signieren
    //
    DndEventTx evt = makeSimpleHitEvt();
    evt.senderPubKey = playerPub;
    dnd::signDndEvent(evt, playerPriv);

    //
    // 6. DnD-Event in generische Transaction einbetten
    //    (wrapDndTxIntoTransaction ist eine globale Funktion, kein dnd:: namespace!)
    //
    Transaction tx = wrapDndTxIntoTransaction(evt);

    //
    // 7. TX zusätzlich generisch signieren (Blockchain-Level)
    //
    tx.sign(playerPriv);

    //
    // 8. TX in den Mempool legen (einfache API)
    //
    mempool.addTransaction(tx);
    ASSERT_EQ(mempool.size(), 1u);

    //
    // 9. Block minen
    //
    auto pending = mempool.getAll();
    Block newBlock = builder.buildBlock(pending);

    bool appended = chain.appendBlock(newBlock);
    ASSERT_TRUE(appended);

    //
    // 10. DnD-State aus Chain rekonstruieren
    //
    DndState state;
    std::string stateErr;
    bool rebuilt = state.rebuildFromChain(chain, stateErr);
    ASSERT_TRUE(rebuilt);

    //
    // 11. Monster-HP testen (optional)
    //     Hinweis: In deinem aktuellen DnD-Code wird kein Monster automatisch
    //     erzeugt, wenn man ein Hit-Event sendet. Deshalb kann dieser Test
    //     *nicht* zuverlässig HP prüfen.
    //
    //     Falls du HP prüfen willst:
    //       → zuerst eine SpawnMonster-TX in die Chain legen.
    //
    //     Deshalb kommentiert:
    //
    // auto it = state.monsters.find("goblin-1");
    // ASSERT_TRUE(it != state.monsters.end());
    // ASSERT_EQ(it->second.hp, <expected>);
}

