#pragma once

#include <string>
#include <vector>

class RejoinController{
public:
    void detectOfflineState();
    void requestSnapshotFromLeader();
    void applyFastSync();
    void resumeConsensus();
};
