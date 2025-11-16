#pragma once

#include <vector>
#include <cstdint>

#include "core/block.hpp"
#include "core/transaction.hpp"

class Blockchain;
class Mempool;

/// Erzeugt signierte Blöcke (Proof-of-Authority, DM-Key)
class BlockBuilder {
public:
    /// chain  = referenz auf Blockchain (für height + prevHash)
    /// dmPriv = DM-Privatkey (zum Signieren der BlockHeader)
    /// dmPub  = DM-Pubkey    (landet im validatorPubKey)
    BlockBuilder(Blockchain& chain,
                 const std::vector<uint8_t>& dmPriv,
                 const std::vector<uint8_t>& dmPub);

    /// Baut einen Block aus einer expliziten Liste von Transaktionen.
    /// - setzt height
    /// - setzt prevHash
    /// - setzt timestamp
    /// - berechnet merkleRoot
    /// - signiert Header mit DM-Key
    Block buildBlock(const std::vector<Transaction>& txs) const;

    /// Komfort: baut Block direkt aus dem Mempool ( Mempool wird NICHT geleert )
    Block buildBlockFromMempool(Mempool& mempool) const;

    /// Komfort: baut Block aus dem Mempool UND hängt ihn an die Chain an.
    /// - bei Erfolg: outBlock = erzeugter Block, mempool wird geleert, return true
    /// - bei Fehler: return false, mempool bleibt unverändert
    bool buildAndAppendFromMempool(Mempool& mempool, Block& outBlock) const;

private:
    Blockchain& chain_;
    std::vector<uint8_t> dmPrivKey_;
    std::vector<uint8_t> dmPubKey_;
};

