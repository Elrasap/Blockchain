#include "tests/testFramework.hpp"
#include "dnd/combat/encounter.hpp"
#include <random>
#include <string>

using dnd::combat::EncounterManager;

TEST_CASE(fuzz_encounter_turns_and_initiative) {
    EncounterManager mgr;

    auto& enc = mgr.startEncounter("Fuzz Fight");

    // Random Engine
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> initDist(1, 20);

    // ein paar Charaktere/Monster rein
    for (int i = 0; i < 5; ++i) {
        mgr.addCharacter(enc.id, "char-" + std::to_string(i), initDist(rng));
        mgr.addMonster(enc.id, "mon-" + std::to_string(i), initDist(rng));
    }

    // Viele Runden nextTurn
    for (int i = 0; i < 1000; ++i) {
        bool ok = mgr.nextTurn(enc.id);
        ASSERT_TRUE(ok);

        dnd::combat::Encounter snap;
        bool got = mgr.get(enc.id, snap);
        ASSERT_TRUE(got);

        ASSERT_TRUE(snap.turnIndex >= 0);
        ASSERT_TRUE(snap.turnIndex < (int)snap.order.size());
        ASSERT_TRUE(snap.round >= 1);
    }
}

