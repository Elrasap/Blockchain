#include "tests/testFramework.hpp"

#include "core/blockBuilder.hpp"
#include "core/blockchain.hpp"
#include "core/crypto.hpp"
#include "core/dmKeyManager.hpp"
#include "core/mempool.hpp"
#include "core/poaValidator.hpp"
#include "core/transactionValidator.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "network/messages.hpp"
#include "network/peerManager.hpp"
#include "network/syncManager.hpp"
#include "storage/blockStore.hpp"

#include <sys/stat.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

namespace {

struct TempDirectory {
    std::filesystem::path path;

    explicit TempDirectory(const std::string& name)
    {
        const auto suffix = std::chrono::steady_clock::now()
            .time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("dnd-blockchain-" + name + "-" + std::to_string(suffix));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

uint64_t nowSeconds()
{
    return static_cast<uint64_t>(std::time(nullptr));
}

Transaction makeTransaction(const dnd::DndEventTx& event,
                            const crypto::KeyPair& signer,
                            uint64_t nonce)
{
    Transaction tx;
    tx.senderPubkey = signer.publicKey;
    tx.payload = dnd::encodeDndTx(event);
    tx.nonce = nonce;
    tx.sign(signer.privateKey);
    return tx;
}

Block appendEvent(Blockchain& chain,
                  const crypto::KeyPair& dm,
                  const dnd::DndEventTx& event,
                  const crypto::KeyPair& signer,
                  uint64_t nonce)
{
    BlockBuilder builder(chain, dm.privateKey, dm.publicKey);
    Block block = builder.buildBlock({makeTransaction(event, signer, nonce)});
    ASSERT_TRUE(chain.appendBlock(block));
    return block;
}

dnd::DndEventTx createCharacter(const std::string& id,
                                const std::vector<uint8_t>& owner)
{
    dnd::DndEventTx event;
    event.eventType = dnd::DndEventType::CreateCharacter;
    event.actorId = id;
    event.actorType = 0;
    event.ownerPubKey = owner;
    event.timestamp = nowSeconds();
    return event;
}

} // namespace

TEST_CASE(transaction_wire_roundtrip_covers_signature)
{
    const auto keys = crypto::generateKeyPair();
    Transaction tx;
    tx.senderPubkey = keys.publicKey;
    tx.payload = {0x10, 0x20, 0x30};
    tx.nonce = 42;
    tx.fee = 7;
    tx.sign(keys.privateKey);

    const auto wire = tx.serialize();
    Transaction decoded;
    std::string error;
    ASSERT_TRUE(Transaction::deserialize(wire, decoded, error));
    ASSERT_EQ(decoded.senderPubkey, tx.senderPubkey);
    ASSERT_EQ(decoded.payload, tx.payload);
    ASSERT_EQ(decoded.signature, tx.signature);
    ASSERT_EQ(decoded.nonce, 42u);
    ASSERT_TRUE(decoded.verifySignature());

    auto tampered = wire;
    tampered[50] ^= 0x01;
    ASSERT_TRUE(Transaction::deserialize(tampered, decoded, error));
    ASSERT_FALSE(decoded.verifySignature());

    auto trailing = wire;
    trailing.push_back(0);
    ASSERT_FALSE(Transaction::deserialize(trailing, decoded, error));
}

TEST_CASE(dnd_event_helper_signs_the_canonical_envelope)
{
    const auto player = crypto::generateKeyPair();
    dnd::DndEventTx event;
    event.eventType = dnd::DndEventType::SkillCheck;
    event.encounterId = "enc";
    event.actorId = "hero";
    event.roll = 14;
    event.timestamp = nowSeconds();
    event.transactionNonce = 9;

    dnd::signDndEvent(event, player.privateKey);
    std::string error;
    ASSERT_EQ(event.senderPubKey, player.publicKey);
    ASSERT_TRUE(dnd::verifyDndEventSignature(event, error));

    ++event.transactionNonce;
    ASSERT_FALSE(dnd::verifyDndEventSignature(event, error));
}

TEST_CASE(block_wire_roundtrip_contains_transactions)
{
    const auto dm = crypto::generateKeyPair();
    Transaction tx;
    tx.senderPubkey = dm.publicKey;
    tx.payload = {1, 2, 3, 4};
    tx.nonce = 1;
    tx.sign(dm.privateKey);

    Block block;
    block.header.height = 1;
    block.header.timestamp = nowSeconds();
    block.transactions.push_back(tx);
    block.header.merkleRoot = block.calculateMerkleRoot();
    ASSERT_TRUE(signBlockHeader(block.header, dm.privateKey, dm.publicKey));

    const auto wire = block.serialize();
    Block decoded;
    std::string error;
    ASSERT_TRUE(Block::deserialize(wire, decoded, error));
    ASSERT_EQ(decoded.transactions.size(), 1u);
    ASSERT_EQ(decoded.transactions.front().signature, tx.signature);
    ASSERT_EQ(decoded.header.merkleRoot, decoded.calculateMerkleRoot());
    ASSERT_TRUE(verifyBlockHeaderSignature(decoded.header));

    auto truncated = wire;
    truncated.pop_back();
    ASSERT_FALSE(Block::deserialize(truncated, decoded, error));
}

TEST_CASE(message_framing_handles_fragmentation_and_coalescing)
{
    const Message first{MessageType::PING, {1, 2, 3}};
    const Message second{MessageType::PONG, {4, 5}};
    const auto firstWire = encodeMessage(first);
    const auto secondWire = encodeMessage(second);

    std::vector<uint8_t> partial(firstWire.begin(), firstWire.begin() + 5);
    Message decoded;
    std::size_t consumed = 0;
    std::string error;
    ASSERT_TRUE(tryDecodeMessageFrame(partial, decoded, consumed, error) ==
                FrameDecodeResult::NeedMoreData);

    std::vector<uint8_t> combined = firstWire;
    combined.insert(combined.end(), secondWire.begin(), secondWire.end());
    ASSERT_TRUE(tryDecodeMessageFrame(combined, decoded, consumed, error) ==
                FrameDecodeResult::Complete);
    ASSERT_EQ(decoded.payload, first.payload);
    combined.erase(combined.begin(), combined.begin() + consumed);
    ASSERT_TRUE(tryDecodeMessageFrame(combined, decoded, consumed, error) ==
                FrameDecodeResult::Complete);
    ASSERT_EQ(decoded.payload, second.payload);

    auto oversized = firstWire;
    oversized[7] = oversized[8] = oversized[9] = oversized[10] = 0xFF;
    ASSERT_TRUE(tryDecodeMessageFrame(oversized, decoded, consumed, error) ==
                FrameDecodeResult::Invalid);
}

TEST_CASE(decoders_reject_random_input_without_crashing)
{
    std::mt19937 generator(1337);
    std::uniform_int_distribution<int> length(0, 256);
    std::uniform_int_distribution<int> byte(0, 255);

    for (int iteration = 0; iteration < 1000; ++iteration) {
        std::vector<uint8_t> input(static_cast<std::size_t>(length(generator)));
        for (auto& value : input)
            value = static_cast<uint8_t>(byte(generator));

        Message message;
        std::size_t consumed = 0;
        std::string error;
        (void)tryDecodeMessageFrame(input, message, consumed, error);

        Transaction tx;
        (void)Transaction::deserialize(input, tx, error);

        Block block;
        (void)Block::deserialize(input, block, error);

        try {
            (void)dnd::decodeDndTx(input);
        } catch (...) {
        }
    }
}

TEST_CASE(dm_and_player_permissions_are_cryptographic)
{
    const auto dm = crypto::generateKeyPair();
    const auto player = crypto::generateKeyPair();
    const auto attacker = crypto::generateKeyPair();
    TransactionValidator validator(dm.publicKey);
    dnd::DndState state;
    std::string error;

    auto create = createCharacter("hero", player.publicKey);
    ASSERT_TRUE(validator.validateAndApply(
        makeTransaction(create, dm, 1), state, error));
    ASSERT_EQ(*state.characterOwner("hero"), player.publicKey);

    auto unauthorizedCreate = createCharacter("intruder", attacker.publicKey);
    ASSERT_FALSE(validator.validateAndApply(
        makeTransaction(unauthorizedCreate, player, 2), state, error));
    ASSERT_FALSE(state.characterExists("intruder"));

    dnd::DndEventTx start;
    start.eventType = dnd::DndEventType::StartEncounter;
    start.encounterId = "enc-1";
    start.timestamp = nowSeconds();
    ASSERT_FALSE(validator.validateAndApply(
        makeTransaction(start, player, 3), state, error));
    ASSERT_TRUE(validator.validateAndApply(
        makeTransaction(start, dm, 4), state, error));

    dnd::DndEventTx initiative;
    initiative.eventType = dnd::DndEventType::Initiative;
    initiative.encounterId = "enc-1";
    initiative.actorId = "hero";
    initiative.actorType = 0;
    initiative.roll = 17;
    initiative.timestamp = nowSeconds();
    ASSERT_FALSE(validator.validateAndApply(
        makeTransaction(initiative, attacker, 5), state, error));
    ASSERT_TRUE(validator.validateAndApply(
        makeTransaction(initiative, player, 6), state, error));
}

TEST_CASE(state_transitions_are_atomic_and_do_not_revive_targets)
{
    dnd::DndState state;
    std::string error;

    auto hero = createCharacter("hero", std::vector<uint8_t>(32, 1));
    auto target = createCharacter("target", std::vector<uint8_t>(32, 2));
    ASSERT_TRUE(state.apply(hero, error));
    ASSERT_TRUE(state.apply(target, error));

    dnd::DndEventTx start;
    start.eventType = dnd::DndEventType::StartEncounter;
    start.encounterId = "enc";
    ASSERT_TRUE(state.apply(start, error));

    dnd::DndEventTx invalid;
    invalid.eventType = dnd::DndEventType::Damage;
    invalid.encounterId = "enc";
    invalid.actorId = "hero";
    invalid.targetId = "missing";
    invalid.damage = 5;
    ASSERT_FALSE(state.apply(invalid, error));
    ASSERT_FALSE(state.characterExists("missing"));
    ASSERT_EQ(state.getEncounter("enc")->events.size(), 1u);

    dnd::DndEventTx damage = invalid;
    damage.targetId = "target";
    damage.damage = 50;
    ASSERT_TRUE(state.apply(damage, error));
    ASSERT_EQ(state.getCharacterHp("target"), 0);
    ASSERT_TRUE(state.apply(damage, error));
    ASSERT_EQ(state.getCharacterHp("target"), 0);
}

TEST_CASE(nodes_converge_after_out_of_order_sync_and_restart)
{
    TempDirectory temp("sync");
    const auto dm = crypto::generateKeyPair();
    const auto player = crypto::generateKeyPair();

    BlockStore authorityStore((temp.path / "authority").string());
    Blockchain authority(authorityStore, dm.publicKey);
    ASSERT_TRUE(authority.ensureGenesisBlock(dm.privateKey));

    std::vector<Block> blocks;
    blocks.push_back(appendEvent(authority, dm,
        createCharacter("hero", player.publicKey), dm, 1));

    dnd::DndEventTx spawn;
    spawn.eventType = dnd::DndEventType::SpawnMonster;
    spawn.actorId = "goblin";
    spawn.actorType = 1;
    spawn.timestamp = nowSeconds();
    blocks.push_back(appendEvent(authority, dm, spawn, dm, 2));

    dnd::DndEventTx start;
    start.eventType = dnd::DndEventType::StartEncounter;
    start.encounterId = "enc";
    start.timestamp = nowSeconds();
    blocks.push_back(appendEvent(authority, dm, start, dm, 3));

    dnd::DndEventTx initiative;
    initiative.eventType = dnd::DndEventType::Initiative;
    initiative.encounterId = "enc";
    initiative.actorId = "hero";
    initiative.roll = 19;
    initiative.timestamp = nowSeconds();
    blocks.push_back(appendEvent(authority, dm, initiative, player, 4));

    BlockStore replicaStore((temp.path / "replica").string());
    ASSERT_TRUE(replicaStore.appendBlock(authority.getBlock(0)));
    Blockchain replica(replicaStore, dm.publicKey);
    PeerManager peers(0);
    SyncManager sync(replica, peers);

    ASSERT_FALSE(sync.handleBlock(blocks[3]));
    ASSERT_FALSE(sync.handleBlock(blocks[2]));
    ASSERT_FALSE(sync.handleBlock(blocks[1]));
    ASSERT_TRUE(sync.handleBlock(blocks[0]));
    ASSERT_EQ(replica.getLatestBlock().hash(), authority.getLatestBlock().hash());
    ASSERT_EQ(replica.getDndState().encounters.at("enc").actors.size(), 1u);

    Blockchain restarted(replicaStore, dm.publicKey);
    ASSERT_EQ(restarted.getLatestBlock().hash(), authority.getLatestBlock().hash());
    ASSERT_EQ(restarted.getDndState().characters.size(), 1u);
    ASSERT_EQ(restarted.getDndState().monsters.size(), 1u);
}

TEST_CASE(dm_key_is_stored_outside_config_with_private_permissions)
{
    TempDirectory temp("key");
    const auto path = temp.path / "keys" / "dm.key";
    DmKeyPair first;
    DmKeyPair second;
    ASSERT_TRUE(loadOrCreateDmKey(path.string(), first));
    ASSERT_TRUE(loadOrCreateDmKey(path.string(), second));
    ASSERT_EQ(first.publicKey, second.publicKey);
    ASSERT_EQ(first.privateKey, second.privateKey);

    struct stat metadata{};
    ASSERT_EQ(::stat(path.c_str(), &metadata), 0);
    ASSERT_EQ(metadata.st_mode & 0777, static_cast<mode_t>(0600));
}

TEST_CASE(mempool_admission_is_thread_safe)
{
    TempDirectory temp("mempool");
    const auto dm = crypto::generateKeyPair();
    BlockStore store((temp.path / "blocks").string());
    Blockchain chain(store, dm.publicKey);
    ASSERT_TRUE(chain.ensureGenesisBlock(dm.privateKey));

    Mempool mempool(
        chain.transactionValidator(),
        [&]() { return chain.getDndState(); },
        [&](const std::array<uint8_t, 32>& hash) {
            return chain.hasTransaction(hash);
        });

    std::vector<Transaction> transactions;
    for (uint64_t nonce = 1; nonce <= 8; ++nonce) {
        Transaction tx;
        tx.senderPubkey = dm.publicKey;
        tx.payload = {0x42, static_cast<uint8_t>(nonce)};
        tx.nonce = nonce;
        tx.sign(dm.privateKey);
        transactions.push_back(std::move(tx));
    }

    std::vector<std::thread> workers;
    std::atomic<bool> accepted{true};
    for (const auto& tx : transactions) {
        workers.emplace_back([&mempool, &accepted, tx]() {
            std::string error;
            if (!mempool.addTransactionValidated(tx, error))
                accepted = false;
        });
    }
    for (auto& worker : workers)
        worker.join();

    ASSERT_TRUE(accepted);
    ASSERT_EQ(mempool.size(), 8u);
}
