#pragma once
#include <vector>
#include "core/block.hpp"
#include "core/transaction.hpp"
#include "core/blockchain.hpp"

// BlockBuilder: baut einen neuen Block auf Basis der aktuellen Chain
// + einer Menge von Transaktionen und signiert ihn mit dem DM-Key.
class BlockBuilder {
public:
    BlockBuilder(Blockchain& chain,
                 const std::vector<uint8_t>& dmPrivKey,
                 const std::vector<uint8_t>& dmPubKey);

    // Baut einen neuen Block, signiert ihn und gibt ihn zurück.
    // Hängt ihn NICHT automatisch an die Chain an.
    Block buildBlock(const std::vector<Transaction>& txs) const;

private:
    Blockchain& chain;
    std::vector<uint8_t> dmPrivKey;
    std::vector<uint8_t> dmPubKey;
};

