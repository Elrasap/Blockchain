#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <optional>

using Term = uint64_t;
using Index = uint64_t;
using TimestampMs = uint64_t;

using Hash32 = std::array<uint8_t, 32>;
using PeerId = std::array<uint8_t, 32>;

enum class ConsensusRole { Leader, Follower, Candidate };

struct Block;
struct BlockHeader;

struct LogEntry {
    Index index;
    Term term;
    Hash32 blockHash;
};

