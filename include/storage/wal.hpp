#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

struct WalEntry {
    std::string type;
    std::vector<uint8_t> data;
    uint64_t timestamp;
};

class WriteAheadLog {
public:
    void append(const WalEntry& entry);
    void flush();
    void replayLog(const std::function<void(const WalEntry&)>& callback);
    void truncateUpTo(uint64_t upto);
    void onCommit(uint64_t index);
    void replayTo(uint64_t commitIndex);
};

