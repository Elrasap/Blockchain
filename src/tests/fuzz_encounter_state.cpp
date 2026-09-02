#include "tests/testFramework.hpp"
#include "dnd/dndState.hpp"

#include <algorithm>
#include <random>
#include <string>

TEST_CASE(fuzz_encounter_initiative_projection)
{
    dnd::DndState state;
    std::string error;

    for (int i = 0; i < 5; ++i) {
        dnd::DndEventTx character;
        character.eventType = dnd::DndEventType::CreateCharacter;
        character.actorId = "char-" + std::to_string(i);
        character.ownerPubKey.assign(32, static_cast<uint8_t>(i + 1));
        ASSERT_TRUE(state.apply(character, error));

        dnd::DndEventTx monster;
        monster.eventType = dnd::DndEventType::SpawnMonster;
        monster.actorId = "mon-" + std::to_string(i);
        monster.actorType = 1;
        ASSERT_TRUE(state.apply(monster, error));
    }

    dnd::DndEventTx start;
    start.eventType = dnd::DndEventType::StartEncounter;
    start.encounterId = "enc-fuzz";
    ASSERT_TRUE(state.apply(start, error));

    std::mt19937 generator(7);
    std::uniform_int_distribution<int> initiative(1, 20);
    for (int i = 0; i < 1000; ++i) {
        const bool monster = (i % 2) != 0;
        dnd::DndEventTx event;
        event.eventType = dnd::DndEventType::Initiative;
        event.encounterId = "enc-fuzz";
        event.actorType = monster ? 1 : 0;
        event.actorId = (monster ? "mon-" : "char-") + std::to_string(i % 5);
        event.roll = initiative(generator);
        ASSERT_TRUE(state.apply(event, error));
    }

    const auto* encounter = state.getEncounter("enc-fuzz");
    ASSERT_TRUE(encounter != nullptr);
    ASSERT_EQ(encounter->actors.size(), 10u);
    ASSERT_TRUE(std::is_sorted(encounter->actors.begin(), encounter->actors.end(),
        [](const auto& left, const auto& right) {
            return left.initiative > right.initiative;
        }));
}
