#include "tests/testFramework.hpp"
#include "dnd/dndState.hpp"
#include "dnd/dndTx.hpp"
#include <string>

using dnd::DndEventTx;
using dnd::DndState;

TEST_CASE(test_apply_hit_reduces_monster_hp) {
    DndState st;
    st.setMonsterHp("goblin-1", 10);

    DndEventTx evt{};
    evt.encounterId = "enc-1";
    evt.actorId     = "hero-1";
    evt.actorType   = 0; // character
    evt.targetId    = "goblin-1";
    evt.targetType  = 1; // monster
    evt.hit         = true;
    evt.damage      = 4;

    std::string err;
    bool ok = st.apply(evt, err);
    ASSERT_TRUE(ok);
    ASSERT_EQ(st.getMonsterHp("goblin-1"), 6);
}

TEST_CASE(test_apply_kills_monster_sets_zero) {
    DndState st;
    st.setMonsterHp("goblin-1", 5);

    DndEventTx evt{};
    evt.encounterId = "enc-1";
    evt.actorId     = "hero-1";
    evt.actorType   = 0;
    evt.targetId    = "goblin-1";
    evt.targetType  = 1;
    evt.hit         = true;
    evt.damage      = 10;

    std::string err;
    bool ok = st.apply(evt, err);
    ASSERT_TRUE(ok);
    ASSERT_EQ(st.getMonsterHp("goblin-1"), 0);
}

TEST_CASE(test_apply_miss_does_not_change_hp) {
    DndState st;
    st.setCharacterHp("hero-1", 20);

    DndEventTx evt{};
    evt.encounterId = "enc-1";
    evt.actorId     = "goblin-1";
    evt.actorType   = 1; // monster
    evt.targetId    = "hero-1";
    evt.targetType  = 0; // character
    evt.hit         = false;
    evt.damage      = 999;

    std::string err;
    bool ok = st.apply(evt, err);
    ASSERT_TRUE(ok);
    ASSERT_EQ(st.getCharacterHp("hero-1"), 20);
}

