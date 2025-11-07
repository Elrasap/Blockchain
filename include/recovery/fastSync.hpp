#pragma once

class FastSync {
public:
    void downloadSnapshot(const Peer& leader);
    void verifySnapshotHash();
    void applySnapshot(State& s);
    void replayAfterSnapshot(uint64_t fromIndex);
};
