#pragma once
#include <vector>
#include <mutex>
#include <string>
#include <map>
#include "core/block.hpp"

enum class CommitStatus { Pending, Committed };

struct CommitEntry {
    std::array<uint8_t, 32> hash;
    uint64_t height;
    CommitStatus status;
};

class CommitLog {
public:
    explicit CommitLog(const std::string& path);
    void append(const CommitEntry& entry);
    void markCommitted(const std::array<uint8_t, 32>& hash);
    CommitEntry getLatest() const;
    std::vector<CommitEntry> loadAll() const;
    void clear();

private:
    std::string filePath;
    mutable std::mutex mtx;
};

