#include "tests/testFramework.hpp"
#include "core/block.hpp"
#include "core/transaction.hpp"
#include "core/crypto.hpp"
#include <vector>
#include <array>
#include <cstdint>

static Transaction makeDummyTx(const std::vector<uint8_t>& pub,
                               const std::vector<uint8_t>& priv,
                               uint64_t nonce,
                               int payloadByte)
{
    Transaction tx;
    tx.senderPubkey = pub;
    tx.nonce = nonce;
    tx.fee   = 0;
    tx.payload = { static_cast<uint8_t>(payloadByte) };
    tx.sign(priv);
    return tx;
}

TEST_CASE(test_blockheader_hash_changes_with_height) {
    BlockHeader h1{};
    BlockHeader h2{};

    h1.height = 1;
    h2.height = 2;

    auto hash1 = h1.hash();
    auto hash2 = h2.hash();

    ASSERT_NE(hash1, hash2);
}

TEST_CASE(test_block_merkle_root_non_empty) {
    // Keypair
    auto kp = crypto::generateKeyPair();

    Block b;
    b.header.height = 1;

    b.transactions.push_back(makeDummyTx(kp.publicKey, kp.privateKey, 1, 0x01));
    b.transactions.push_back(makeDummyTx(kp.publicKey, kp.privateKey, 2, 0x02));

    auto root = b.calculateMerkleRoot();
    ASSERT_TRUE(root != std::array<uint8_t, 32>{}); // nicht alles 0

    b.header.merkleRoot = root;
    ASSERT_EQ(b.header.merkleRoot, b.calculateMerkleRoot());
}

TEST_CASE(test_block_hash_delegates_to_header) {
    Block b;
    b.header.height = 42;
    auto headerHash = b.header.hash();
    auto blockHash  = b.hash();

    ASSERT_EQ(headerHash, blockHash);
}

