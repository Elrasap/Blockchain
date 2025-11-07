#pragma once
#include <string>
#include <vector>
#include "types.hpp"

enum class RaftMessageType {
    RequestVote,
    RequestVoteResponse,
    AppendEntries,
    AppendEntriesResponse
};

struct RequestVote {
    Term term;
    PeerId candidateId;
    Index lastLogIndex;
    Term lastLogTerm;
};

struct RequestVoteResponse {
    Term term;
    bool voteGranted;
};

struct AppendEntries {
    Term term;
    PeerId leaderId;
    Index prevLogIndex;
    Term prevLogTerm;
    std::vector<LogEntry> entries;
    Index leaderCommit;
};

struct AppendEntriesResponse {
    Term term;
    bool success;
    Index matchIndex;
};

struct RaftMessage {
    RaftMessageType type;
    RequestVote rv;
    RequestVoteResponse rvr;
    AppendEntries ae;
    AppendEntriesResponse aer;
};

