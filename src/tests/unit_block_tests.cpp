#include "tests/testFramework.hpp"
#include "core/block.hpp"
#include "core/transaction.hpp"
#include <array>
#include <vector>

// Hilfsfunktion für Dummy-TXs
static Transaction makeDummyTx(uint32_t n) {
    Transaction tx;
    tx.payload = { uint8_t(n & 0xFF), uint8_t((n >> 8) & 0xFF) };
    return tx;
}

TEST_CASE(test_block_merkle_root_non_empty) {
    Block b;
    b.transactions.push_back(makeDummyTx(1));
    b.transactions.push_back(makeDummyTx(2));
    b.transactions.push_back(makeDummyTx(3));

    auto root = b.calculateMerkleRoot();

    // FIX: kein ASSERT_TRUE mit Komma im Typ → stattdessen ASSERT_NE
    constexpr std::array<uint8_t,32> EMPTY = {};
    ASSERT_NE(root, EMPTY);

}

TEST_CASE(test_block_merkle_root_matches_header_after_set) {
    Block b;
    b.transactions.push_back(makeDummyTx(42));

    auto root = b.calculateMerkleRoot();
    b.header.merkleRoot = root;

    ASSERT_EQ(b.header.merkleRoot, b.calculateMerkleRoot());
}

TEST_CASE(test_block_hash_delegates_to_header) {
    Block b;
    b.transactions.push_back(makeDummyTx(9));

    b.header.merkleRoot = b.calculateMerkleRoot();

    auto headerHash = b.header.hash();
    auto blockHash  = b.hash();

    ASSERT_EQ(headerHash, blockHash);
}

