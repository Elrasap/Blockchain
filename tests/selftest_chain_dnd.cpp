#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"

#include "dnd/dndTxCodec.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndPayload.hpp"
#include "dnd/dndState.hpp"
#include "dnd/dndTxValidator.hpp"

#include "storage/blockStore.hpp"

#include <cassert>
#include <iostream>

TEST(selftest_chain_with_single_dnd_tx)
{
    // Temp-Ordner für Testchain
    BlockStore store("test_blocks");
    store.clear();

    // Fake DM key
    std::vector<uint8_t> pub = {1,2,3};
    std::vector<uint8_t> priv = {9,9,9};

    Blockchain chain(store, pub);
    chain.ensureGenesisBlock(priv);
    chain.rebuildState();

    // DnD validator mock
    dnd::DndValidationContext ctx;
    ctx.characterExists = [&](const std::string&) { return true; };
    ctx.monsterExists   = [&](const std::string&) { return true; };
    ctx.encounterActive = [&](const std::string&) { return true; };
    ctx.hasControlPermission = [&](const std::string&,
                                   const std::vector<uint8_t>&,
                                   bool) { return true; };
    dnd::DndTxValidator validator(ctx);

    Mempool mp(&validator);

    // Build DnD TX
    dnd::DndEventTx evt;
    evt.encounterId = "enc1";
    evt.actorId = "hero1";
    evt.actorType = 0;
    evt.targetId = "goblin";
    evt.targetType = 1;
    evt.damage = 5;
    evt.hit = true;
    evt.timestamp = 123;

    // Sign by DM
    dnd::signDndEvent(evt, priv);

    Transaction tx;
    tx.payload = dnd::encodeDndTx(evt);
    tx.senderPubkey = evt.senderPubKey;
    tx.signature = evt.signature;

    std::string err;
    bool ok = mp.addTransactionValidated(tx, err);
    assert(ok == true);

    BlockBuilder bb(chain, priv, pub);
    Block out;

    bool mined = bb.buildAndAppendFromMempool(mp, out);
    assert(mined == true);

    assert(chain.getHeight() == 1);
}

