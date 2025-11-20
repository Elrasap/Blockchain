#include "core/block.hpp"
#include <cassert>
#include <vector>

TEST(test_block_merkle_root_non_empty)
{
    Block b;

    Transaction t;
    t.payload = {1,2,3};
    b.transactions.push_back(t);

    auto root = b.calculateMerkleRoot();
    assert(root != std::array<uint8_t,32>{});
}

TEST(test_block_merkle_root_matches_header_after_set)
{
    Block b;

    Transaction t;
    t.payload = {4,5,6};
    b.transactions.push_back(t);

    auto root = b.calculateMerkleRoot();
    b.header.merkleRoot = root;

    assert(b.header.merkleRoot == root);
}

TEST(test_block_hash_delegates_to_header)
{
    Block b;
    auto h1 = b.header.hash();
    auto h2 = b.hash();

    assert(h1 == h2);
}

