#include "upgrade/stateValidator.hpp"
#include <fstream>      // <- nötig für std::ifstream
#include <iterator>     // <- nötig für std::istreambuf_iterator

static std::array<uint8_t,32> concatHash(const std::array<uint8_t,32>& a, const std::array<uint8_t,32>& b) {
    std::vector<uint8_t> buf;
    buf.reserve(64);
    buf.insert(buf.end(), a.begin(), a.end());
    buf.insert(buf.end(), b.begin(), b.end());
    return crypto::sha256(buf);
}

std::array<uint8_t,32> StateValidator::computeStateRoot(const std::vector<Block>& blocks) {
    if (blocks.empty()) return crypto::sha256(std::vector<uint8_t>{});
    std::vector<std::array<uint8_t,32>> level;
    level.reserve(blocks.size());
    for (const auto& b : blocks) level.push_back(b.hash());
    while (level.size() > 1) {
        std::vector<std::array<uint8_t,32>> next;
        next.reserve((level.size()+1)/2);
        for (size_t i = 0; i < level.size(); i += 2) {
            const auto& left = level[i];
            const auto& right = (i + 1 < level.size()) ? level[i+1] : level[i];
            next.push_back(concatHash(left, right));
        }
        level.swap(next);
    }
    return level[0];
}

bool StateValidator::compareSnapshots(const std::string& snapAPath, const std::string& snapBPath) {
    std::ifstream a(snapAPath, std::ios::binary);
    std::ifstream b(snapBPath, std::ios::binary);
    if (!a || !b) return false;

    std::vector<uint8_t> ba((std::istreambuf_iterator<char>(a)), std::istreambuf_iterator<char>());
    std::vector<uint8_t> bb((std::istreambuf_iterator<char>(b)), std::istreambuf_iterator<char>());
    return crypto::sha256(ba) == crypto::sha256(bb);
}

bool StateValidator::verifyStateEquality(const std::vector<Block>& chainA, const std::vector<Block>& chainB) {
    return computeStateRoot(chainA) == computeStateRoot(chainB);
}

