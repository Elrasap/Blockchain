#include "dnd/character.hpp"
#include <iostream>

int main() {
    using namespace dnd;

    PlayerAccount p;
    p.address = "addr_123";
    p.publicKeyHex = "abcd1234...";
    p.displayName = "Alice";
    p.isDungeonMaster = false;
    p.permissionLevel = 0;

    CharacterSheet c = makeDefaultCharacter(
        "char_1",
        p.address,
        "Elara",
        CharacterClass::Wizard,
        Race::Elf
    );
    c.stats.intl = 16;
    c.stats.dex = 14;
    c.inventory.push_back("Spellbook");
    c.inventory.push_back("Dagger");

    std::string payload = serializeCharacter(c);

    std::cout << payload << "\n";

    // und wieder zurück:
    CharacterSheet c2 = deserializeCharacter(payload);
    std::cout << "Name: " << c2.name << " Level: " << c2.level << "\n";
}

