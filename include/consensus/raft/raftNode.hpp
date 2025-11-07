#pragma once
#include <string>
#include <map>
#include <optional>
#include "types.hpp"
#include "consensus/raft/raftLog.hpp"
#include "consensus/raft/raftMessage.hpp"

class RaftNode {
public:
    Term currentTerm;
    std::optional<PeerId> votedFor;
    RaftLog log;
    Index commitIndex;
    Index lastApplied;
    ConsensusRole role;
    std::map<PeerId, Index> nextIndex;
    std::map<PeerId, Index> matchIndex;
};

