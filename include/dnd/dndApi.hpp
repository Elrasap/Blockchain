#pragma once
#include <string>
#include <vector>
#include <cstdint>

class DnDApi {
public:
    std::string rollCommit(const std::string& playerId,
                           const std::string& rollCommit);

    bool rollReveal(const std::string& playerId,
                    uint64_t rollValue,
                    const std::string& salt);

    void modifyHp(const std::string& characterId,
                  int32_t delta,
                  const std::string& reason);

    bool transferGold(const std::string& fromId,
                      const std::string& toId,
                      uint64_t amount);

    std::string getCharacter(const std::string& characterId,
                             const std::string& role);

    std::string getEncounters(const std::string& role);

    std::string getEventLog(const std::string& role,
                            uint32_t limit = 100);
};

