#pragma once
#include "block.hpp"
#include "mempool.hpp"
#include "storage/snapshot.hpp"
#include "storage/wal.hpp"
#include "stateMachine.hpp"
#include "state.hpp"
#include "transaction.hpp"
#include "types.hpp"

class CommitPipeline {
public:
    void onNewTx(const Transaction& tx);
    void buildBlock(uint64_t limit);
    void onBlockCommitted(const Block& block);
    void syncFromSnapshot(const Snapshot& snapshot);
};

