#include "core/blockBuilder.hpp"
#include "core/blockchain.hpp"
#include "core/mempool.hpp"
#include "core/block.hpp"
#include <ctime>
#include <iostream>

BlockBuilder::BlockBuilder(Blockchain& chain,
                           const std::vector<uint8_t>& dmPriv,
                           const std::vector<uint8_t>& dmPub)
    : chain_(chain),
      dmPrivKey_(dmPriv),
      dmPubKey_(dmPub)
{
}

// ---------------------------------------------------------
// Block aus expliziter Tx-Liste bauen
// ---------------------------------------------------------
Block BlockBuilder::buildBlock(const std::vector<Transaction>& txs) const
{
    Block b;

    // 1) Header: Höhe bestimmen
    //
    // getHeight() gibt die Höhe des letzten Blocks zurück.
    // Neuer Block = letzterHeight + 1
    uint64_t currentHeight = chain_.getHeight();
    b.header.height = currentHeight + 1;

    // 2) prevHash setzen
    //
    // Wir erwarten, dass immer mindestens der Genesis-Block existiert,
    // weil main() vorher ensureGenesisBlock(...) aufruft.
    Block prev = chain_.getLatestBlock();

    // Falls die Chain noch "leer" wäre (nur Default-Block),
    // setzen wir prevHash = 0…0, ansonsten Hash des Vorgängers.
    bool prevIsDefault =
        (prev.header.timestamp == 0 &&
         prev.header.height == 0 &&
         prev.header.merkleRoot == std::array<uint8_t, 32>{});

    if (prevIsDefault && currentHeight == 0) {
        // Es gibt keinen echten Vorgänger → z.B. wir bauen Genesis
        b.header.prevHash.fill(0);
    } else {
        b.header.prevHash = prev.hash();
    }

    // 3) Timestamp
    b.header.timestamp = static_cast<uint64_t>(std::time(nullptr));

    // 4) Transaktionen einhängen
    b.transactions = txs;

    // 5) Merkle-Root berechnen
    b.header.merkleRoot = b.calculateMerkleRoot();

    // 6) Header mit DM-Key signieren (Proof-of-Authority)
    if (!signBlockHeader(b.header, dmPrivKey_, dmPubKey_)) {
        std::cerr << "[BlockBuilder] signBlockHeader() failed – header left unsigned\n";
    }

    return b;
}

// ---------------------------------------------------------
// Block direkt aus Mempool bauen (ohne anhängen, ohne clear)
// ---------------------------------------------------------
Block BlockBuilder::buildBlockFromMempool(Mempool& mempool) const
{
    auto txs = mempool.getAll();
    return buildBlock(txs);
}

// ---------------------------------------------------------
// Block aus Mempool bauen, an Chain anhängen, Mempool leeren
// ---------------------------------------------------------
bool BlockBuilder::buildAndAppendFromMempool(Mempool& mempool,
                                             Block& outBlock) const
{
    auto txs = mempool.getAll();
    if (txs.empty()) {
        std::cerr << "[BlockBuilder] No transactions in mempool\n";
        return false;
    }

    Block b = buildBlock(txs);

    // Blockchain kümmert sich um PoA-Validierung und Persistenz
    if (!chain_.appendBlock(b)) {
        std::cerr << "[BlockBuilder] chain.appendBlock() failed\n";
        return false;
    }

    // Nur bei Erfolg: Mempool leeren und Block herausgeben
    mempool.clear();
    outBlock = std::move(b);
    return true;
}

