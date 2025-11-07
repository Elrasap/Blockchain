#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "core/block.hpp"
#include "storage/blockStore.hpp"
#include "storage/commitLog.hpp"

class SnapshotManager {
public:
    explicit SnapshotManager(BlockStore* store);
    void createSnapshot();
    bool restoreFromSnapshot();
    std::string latestSnapshotFile() const;
    void clear();

private:
    BlockStore* store;
    std::string snapshot_dir;
    mutable std::mutex mtx;
};

