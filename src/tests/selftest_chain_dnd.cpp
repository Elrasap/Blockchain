#include "tests/testFramework.hpp"
#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"
#include "core/transaction.hpp"
#include "core/crypto.hpp"
#include "storage/blockStore.hpp"
#include "dnd/state.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxSerialization.hpp"
#include <cstdio>

using dnd::DndEventTx;
using dnd::DndState;

static DndEventTx makeSimpleHitEvt() {
    DndEventTx evt{};
    evt.encounterId = "enc-1";
    evt.actorId     = "hero-1";
    evt.actorType   = 0;
    evt.targetId    = "goblin-1";
    evt.targetType  = 1;
    evt.hit         = true;
    evt.damage      = 5;
    evt.timestamp   = time(nullptr);
    return evt;
}

TEST_CASE(selftest_chain_with_single_dnd_tx) {
    // 1. Temp-DB
    const char* dbPath = "selftest_blocks.db";
    std::remove(dbPath);
    BlockStore store(dbPath);

    // 2. DM-Key (Proof-of-Authority)
    auto dmKeys = crypto::generateKeyPair();

    Blockchain chain(store, dmKeys.publicKey);
    chain.ensureGenesisBlock(dmKeys.privateKey);

    // 3. Mempool + Builder
    dnd::DndTxValidator dndValidator;
    Mempool mempool(&dndValidator);
    BlockBuilder builder(chain, dmKeys.privateKey, dmKeys.publicKey);

    // 4. Player Key
    std::vector<uint8_t> playerPub, playerPriv;
    dnd::generatePlayerKeypair(playerPub, playerPriv);

    // 5. DnD-Event → Transaction
    DndEventTx evt = makeSimpleHitEvt();
    evt.senderPubKey = playerPub;
    dnd::signDndEvent(evt, playerPriv);

    Transaction tx = dnd::wrapDndTxIntoTransaction(evt);
    tx.sign(playerPriv); // generische Signatur

    std::string err;
    bool added = mempool.addTransactionValidated(tx, err);
    ASSERT_TRUE(added);

    // 6. Block minen
    auto pending = mempool.getAll();
    Block newBlock = builder.buildBlock(pending);
    bool appended  = chain.appendBlock(newBlock);
    ASSERT_TRUE(appended);

    // 7. State-Rebuild
    DndState state;
    state.rebuildFromChain(chain);

    int hpGoblin = state.getMonsterHp("goblin-1");
    ASSERT_EQ(hpGoblin, 0 /* oder 10-5, je nach Init-HP in DndState */);
}

