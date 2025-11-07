#pragma once
#include <string>
#include <vector>
#include <optional>
#include "types.hpp"

class RaftLog {
public:
    bool append(const LogEntry& entry);
    std::optional<LogEntry> get(const Index index) const;
    Index lastIndex() const;
    void commit(const Index index);
    void truncate(const Index index);
};

