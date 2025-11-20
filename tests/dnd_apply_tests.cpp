#include "dnd/dndState.hpp"
#include "dnd/dndTx.hpp"
#include <cassert>

TEST(test_apply_hit_reduces_monster_hp)
{
    dnd::DndState st;
    std::string err;

    // Monster anlegen
    st.monsters["goblin"].hp = 10;

    dnd::DndEventTx hit;
    hit.encounterId = "enc1";
    hit.actorId     = "hero1";
    hit.actorType   = 0;
    hit.targetId    = "goblin";
    hit.targetType  = 1;
    hit.damage      = 4;
    hit.hit         = true;

    st.apply(hit, err);

    assert(st.monsters["goblin"].hp == 6);
}

TEST(test_apply_kills_monster_sets_zero)
{
    dnd::DndState st;
    std::string err;

    st.monsters["goblin"].hp = 5;

    dnd::DndEventTx hit;
    hit.encounterId = "enc1";
    hit.targetId = "goblin";
    hit.targetType = 1;
    hit.damage = 50;
    hit.hit = true;

    st.apply(hit, err);

    assert(st.monsters["goblin"].hp == 0);
}

TEST(test_apply_miss_does_not_change_hp)
{
    dnd::DndState st;
    std::string err;

    st.monsters["goblin"].hp = 20;

    dnd::DndEventTx miss;
    miss.encounterId = "x";
    miss.targetId = "goblin";
    miss.targetType = 1;
    miss.hit = false;
    miss.damage = 99;

    st.apply(miss, err);

    assert(st.monsters["goblin"].hp == 20);
}

