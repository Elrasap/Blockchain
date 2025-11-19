#include "tests/testFramework.hpp"
#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/dndTxValidator.hpp"
#include "storage/blockStore.hpp"

#include <ctime>

// Wir registrieren den Test ganz normal:
TEST_CASE(selftest_chain_with_single_dnd_tx)
{
    // ---- Blockchain Setup ----
    BlockStore store(":memory:");

    // Dummy-DM Keys – Länge ist hier egal, weil wir appendBlock NICHT aufrufen
    std::vector<uint8_t> dmPub  = {1,2,3};
    std::vector<uint8_t> dmPriv = {9,9,9};

    Blockchain chain(store, dmPub);
    ASSERT_TRUE(chain.ensureGenesisBlock(dmPriv));

    // ---- Validator Setup ----
    dnd::DndValidationContext ctx;

    ctx.characterExists = [&](const std::string&) { return true; };
    ctx.monsterExists   = [&](const std::string&) { return true; };
    ctx.encounterActive = [&](const std::string&) { return true; };

    ctx.hasControlPermission = [&](const std::string&,
                                   const std::vector<uint8_t>&,
                                   bool) { return true; };

    dnd::DndTxValidator validator(ctx);

    // ---- Mempool ----
    Mempool mempool(&validator);

    // ---- TX erstellen ----
    dnd::DndEventTx evt;
    evt.encounterId = "enc1";
    evt.actorId     = "hero1";
    evt.actorType   = 0;
    evt.timestamp   = static_cast<uint64_t>(time(nullptr));

    evt.senderPubKey = dmPub;
    evt.signature    = {9,9,9};  // für diesen Test egal

    Transaction tx;
    tx.payload      = dnd::encodeDndTx(evt);
    tx.senderPubkey = evt.senderPubKey;
    tx.signature    = evt.signature;

    std::string err;
    ASSERT_TRUE(mempool.addTransactionValidated(tx, err));

    // ---- BlockBuilder ----
    BlockBuilder builder(chain, dmPriv, dmPub);

    // WICHTIG: wir bauen NUR den Block,
    // hängen ihn aber NICHT an die Chain (wegen PoA-Signaturcheck).
    Block out = builder.buildBlockFromMempool(mempool);

    // ---- Prüfen ----
    ASSERT_EQ(out.transactions.size(), 1u);

    // Optional: die Chain selbst bleibt nur Genesis hoch:
    ASSERT_EQ(chain.getHeight(), 0u);

    // Falls du sicherstellen willst, dass Genesis existiert:
    auto latest = chain.getLatestBlock();
    ASSERT_EQ(latest.header.height, 0u);

    // Keine zusätzliche PASS-Macro nötig, der Test ist bei Ende "OK"
}

