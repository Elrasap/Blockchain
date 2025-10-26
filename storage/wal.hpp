#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct WalEntry {
    std::string type;
    std::vector<uint8_t> data;
    uint64_t timestamp;
};

class WriteAheadLog {
public:
    void append(const WalEntry& entry);
    void flush();
    void replay(void (*callback)(const WalEntry&));
    void truncate(uint64_t upTo);
};
