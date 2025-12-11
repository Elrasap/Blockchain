#pragma once

#include <vector>
#include <cstdint>

#include "core/block.hpp"
#include "core/transaction.hpp"

class Blockchain;
class Mempool;


class BlockBuilder {
public:
    BlockBuilder(Blockchain& chain,
                 const std::vector<uint8_t>& dmPriv,
                 const std::vector<uint8_t>& dmPub);

    Block buildBlock(const std::vector<Transaction>& txs) const;

    Block buildBlockFromMempool(Mempool& mempool) const;

    bool buildAndAppendFromMempool(Mempool& mempool, Block& outBlock) const;

private:
    Blockchain& chain_;
    std::vector<uint8_t> dmPrivKey_;
    std::vector<uint8_t> dmPubKey_;
};

