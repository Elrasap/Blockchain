#pragma once
#include <string>
#include <vector>
#include <memory>

class RollCommitReveal {
public:
    std::string commitRoll(const std::string& playerId, uint64_t rollValue, const std::string& salt);
    bool revealRoll(const std::string& playerId, uint64_t rollValue, const std::string& salt);
    uint64_t computeResult(const std::string& playerId);
};
