#include "tests/testFramework.hpp"
#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/dndTxValidator.hpp"
#include "storage/blockStore.hpp"
#include "core/crypto.hpp"

#include <ctime>

TEST_CASE(selftest_chain_with_single_dnd_tx)
{
    // ---- Blockchain Setup ----
    BlockStore store(":memory:");

    // Echte gültige Keys
    crypto::KeyPair keypair = crypto::generateKeyPair();
    std::vector<uint8_t> dmPub  = keypair.publicKey;
    std::vector<uint8_t> dmPriv = keypair.privateKey;

    Blockchain chain(store, dmPub);
    ASSERT_TRUE(chain.ensureGenesisBlock(dmPriv));

    // ---- Validator Setup ----
    dnd::DndValidationContext ctx;

    ctx.characterExists = [&](const std::string&) {
        return true;
    };
    ctx.monsterExists = [&](const std::string&) {
        return true;
    };
    ctx.encounterActive = [&](const std::string&) {
        return true;
    };
    ctx.hasControlPermission = [&](const std::string&,
                                   const std::vector<uint8_t>&,
                                   bool) {
        return true;
    };

    dnd::DndTxValidator validator(ctx);

    // ---- Mempool ----
    Mempool mempool(&validator);
    mempool.ignoreSignatureCheck = true;


    // ---- TX erstellen ----
    dnd::DndEventTx evt;
    evt.encounterId = "enc1";
    evt.actorId     = "hero1";
    evt.actorType   = 0;
    evt.timestamp   = static_cast<uint64_t>(time(nullptr));

    // ===== WICHTIG: Target hinzufügen ====
    evt.targetId    = "monster1";
    evt.targetType  = 1;

    evt.senderPubKey = dmPub;
    evt.signature    = crypto::sign({1,2,3}, dmPriv);

    Transaction tx;
    tx.payload      = dnd::encodeDndTx(evt);
    tx.senderPubkey = evt.senderPubKey;
    tx.signature    = evt.signature;

    std::string err;
    bool ok = mempool.addTransactionValidated(tx, err);
    if (!ok) {
        printf("[VALIDATION ERROR] %s\n", err.c_str());
    }
    ASSERT_TRUE(ok);


    // ---- BlockBuilder ----
    BlockBuilder builder(chain, dmPriv, dmPub);

    // KEIN append → PoA Signature wird nicht geprüft
    Block out = builder.buildBlockFromMempool(mempool);

    // ---- Prüfen ----
    ASSERT_EQ(out.transactions.size(), 1u);
    ASSERT_EQ(chain.getHeight(), 0u);

    auto latest = chain.getLatestBlock();
    ASSERT_EQ(latest.header.height, 0u);
}

