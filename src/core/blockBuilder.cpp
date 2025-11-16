#include "core/blockBuilder.hpp"
#include "core/crypto.hpp"
#include <iostream>
#include <chrono>
#include "core/transaction.hpp"
#include "core/block.hpp"

using namespace std;

namespace {

uint64_t nowMs() {
    using namespace std::chrono;
    auto now = system_clock::now().time_since_epoch();
    return duration_cast<milliseconds>(now).count();
}

} // namespace

BlockBuilder::BlockBuilder(Blockchain& chain,
                           const std::vector<uint8_t>& dmPrivKey,
                           const std::vector<uint8_t>& dmPubKey)
    : chain(chain),
      dmPrivKey(dmPrivKey),
      dmPubKey(dmPubKey) {}

Block BlockBuilder::buildBlock(const std::vector<Transaction>& txs) const {
    Block prev = chain.getLatestBlock();

    Block b;
    b.transactions = txs;

    // Header-Felder setzen
    b.header.height    = prev.header.height + 1;
    b.header.timestamp = nowMs();
    b.header.prevHash  = prev.hash();
    b.header.merkleRoot = b.calculateMerkleRoot();

    // Signieren (Proof-of-Authority)
    signBlockHeader(b.header, dmPrivKey, dmPubKey);

    return b;
}

