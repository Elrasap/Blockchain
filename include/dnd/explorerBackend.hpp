#pragma once
#include <string>
#include <vector>
#include <memory>

class PrivacyFilter {
public:
    std::vector<Block> getRecentBlocks();
    std::vector<Transaction> getRecentTxs();
    std::vector<std::string> getCurrentEncounters();
    std::string getCharacterState(const std::string& id);

};
