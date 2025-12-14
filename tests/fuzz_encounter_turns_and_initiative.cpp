#include "dnd/dndState.hpp"
#include "dnd/dndTx.hpp"
#include <cassert>
#include <string>

TEST(fuzz_encounter_turns_and_initiative)
{
    dnd::DndState st;
    std::string err;

    // Encounter erzeugen
    dnd::DndEventTx start;
    start.encounterId = "enc1";
    start.actorId = "dm";
    start.actorType = 0;
    start.timestamp = 1;
    st.apply(start, err); // -> startEncounter analogue

    // Initiative-Random Test
    for (int i = 0; i < 100; i++) {
        dnd::DndEventTx init;
        init.encounterId = "enc1";
        init.actorId = "hero";
        init.actorType = 0;
        init.roll = rand() % 20;
        init.timestamp = 2 + i;
        st.apply(init, err);
    }

    assert(st.encounters.contains("enc1"));
    assert(st.encounters["enc1"].round >= 0);
}

