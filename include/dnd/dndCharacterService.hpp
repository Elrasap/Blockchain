#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "dnd/character.hpp"
#include "dnd/payload.hpp"
#include "dnd/patch.hpp"

namespace dnd {

class DndCharacterService {
public:
    std::unordered_map<std::string, CharacterSheet> characters;
    std::unordered_map<std::string, std::vector<nlohmann::json>> history;

    template<typename Block>
    void applyBlocks(const std::vector<Block>& blocks) {
        for (const auto& b : blocks) {
            applyTransactions(b.transactions);
        }
    }

    template<typename Tx>
    void applyTransactions(const std::vector<Tx>& txs) {
        for (const auto& t : txs) {
            applyTransaction(t);
        }
    }

    template<typename Tx>
    void applyTransaction(const Tx& t) {
        if (!t.isDndTransaction()) return;
        DndPayload p = deserializePayload(t.payload);
        if (p.type == PayloadType::DND_CREATE_CHARACTER) {
            handleCreate(p.jsonData);
        } else if (p.type == PayloadType::DND_UPDATE_CHARACTER) {
            handleUpdate(p.jsonData);
        }
    }

    void applyCreateJson(const std::string& jsonData);
    void applyUpdateJson(const std::string& jsonData);

    std::vector<CharacterSheet> listCharacters() const;
    bool getCharacter(const std::string& id, CharacterSheet& out) const;
    void logDnDTx(const std::string& jsonString);


private:
    void handleCreate(const std::string& data);
    void handleUpdate(const std::string& data);
};

}

