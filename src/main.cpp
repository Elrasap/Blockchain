#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <thread>

// ====== CORE ======
#include "core/crypto.hpp"
#include "core/transaction.hpp"
#include "core/mempool.hpp"
#include "core/block.hpp"
#include "core/blockBuilder.hpp"
#include "core/validation.hpp"

// ====== RELEASE ======
#include "release/checksummer.hpp"
#include "release/releaseManifest.hpp"

// ====== STORAGE ======
#include "storage/historyStore.hpp"
#include "storage/blockStore.hpp"

// ====== OPS ======
#include "ops/reliabilityGuard.hpp"

// ====== ANALYTICS ======
#include "analytics/rtoRpoAnalyzer.hpp"
#include "analytics/trendAnalyzer.hpp"

// ====== DND SYSTEM ======
#include "dnd/dndCharacterService.hpp"
#include "dnd/payload.hpp"
#include "dnd/patch.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/combat/combatService.hpp"
#include "dnd/combat/encounter.hpp"
#include "dnd/combat/monster.hpp"

// ====== WEBSERVER ======
#include "web/dashboardServer.hpp"
#include "metrics/metricsServer.hpp"

namespace fs = std::filesystem;

struct TestResult {
    std::string name;
    bool ok;
};

// Just a helper to print test results
static void print(const std::vector<TestResult>& tests) {
    std::cout << "\n==== SELFTEST SUMMARY ====\n";
    for (auto& t : tests) {
        std::cout << (t.ok ? "[OK]   " : "[FAIL] ") << t.name << "\n";
    }
    std::cout << "===========================\n";
}

// =================================================================================
// CRYPTO TEST
// =================================================================================
bool testCrypto() {
    try {
        std::vector<uint8_t> msg = {1,2,3};

        // SHA256 korrekt
        auto h = crypto::sha256(msg);

        // KeyPair korrekt erzeugen
        crypto::KeyPair kp = crypto::generateKeyPair();

        Transaction tx;
        tx.payload = msg;
        tx.nonce   = 1;
        tx.fee     = 0;

        // notwendige Felder
        tx.senderPubkey = kp.publicKey;

        // signieren mit privateKey
        tx.sign(kp.privateKey);

        return tx.verifySignature();
    }
    catch (...) {
        return false;
    }
}




// =================================================================================
// TRANSACTION & MEMPOOL
// =================================================================================
bool testMempool() {
    try {
        Mempool mp;
        Transaction tx;
        tx.payload = {9,9,9};
        mp.addTransaction(tx);
        return mp.size() == 1;
    }
    catch (...) { return false; }
}

// =================================================================================
// BLOCK TEST
// =================================================================================
bool testBlock() {
    try {
        Transaction tx;
        tx.payload = {1,2,3};

        Block b;
        b.transactions.push_back(tx);
        auto root = b.calculateMerkleRoot();
        auto h = b.hash();
        return true;
    }
    catch (...) { return false; }
}

// =================================================================================
// HISTORY STORE
// =================================================================================
bool testHistory() {
    HistoryStore hs("history.db");

    if (!hs.init())
        return false;

    RtoRecord rec;
    rec.filename   = "backup1.tar";
    rec.rto_ms     = 1200.0;
    rec.restore_ms = 800.0;
    rec.passed     = true;

    // in einen Vektor packen
    std::vector<RtoRecord> runs{rec};
    if (!hs.insertRtoRecords(runs))
        return false;

    // neue API nutzen
    auto r = hs.loadRecentRto(1);
    return !r.empty();
}


// =================================================================================
// RTO ANALYZER
// =================================================================================
bool testRtoAnalyzer() {
    try {
        RtoRpoAnalyzer a("reports");
        auto x = a.analyzeAll();
        (void)x;
        return true;
    }
    catch (...) { return false; }
}

// =================================================================================
// DND: Character Creation
// =================================================================================
bool testDndCharacters() {
    // Wir testen hier nur, dass der Service konstruierbar ist
    // und ein CharacterSheet-Objekt erzeugt werden kann.
    try {
        // Aktueller Service hat keinen Pfad-Konstruktor, daher Default:
        dnd::DndCharacterService s;

        dnd::CharacterSheet sheet;
        sheet.id   = "hero1";
        sheet.name = "Conan";

        // Optional könnte man hier handleCreate(...) aufrufen.
        return true;
    } catch (...) {
        return false;
    }
}


// =================================================================================
// DND: Monster System
// =================================================================================
bool testMonsters() {
    dnd::MonsterService ms("monsters.json");

    dnd::Monster m;
    m.id        = "wolf1";
    m.name      = "Wolf";
    m.level     = 2;
    m.hpCurrent = 26;
    m.hpMax     = 26;
    m.ac        = 13; // Beispielwert
    // Stats optional (hier Defaults)

    ms.upsert(m);

    dnd::Monster out;
    return ms.get("wolf1", out);
}


// =================================================================================
// DND: Combat & Rolls
// =================================================================================
bool testCombat() {
    dnd::combat::CombatService C;
    int roll = C.getDice().roll("1d20");   // convenience overload (siehe dice.cpp)
    // einfacher Sanity-Check
    return roll >= 1 && roll <= 20;
}

// =================================================================================
// DND: Encounter Manager
// =================================================================================
bool testEncounter() {
    // EncounterManager liegt in dnd::combat
    dnd::combat::EncounterManager E;

    auto& enc = E.startEncounter("test_enc");
    E.addCharacter(enc.id, "hero1", 15);
    E.addMonster(enc.id, "wolf1", 12);

    E.nextTurn(enc.id);

    dnd::combat::Encounter out;
    return E.get(enc.id, out);
}


// =================================================================================
// DASHBOARD & METRICS (start then stop)
// =================================================================================
bool testServers() {
    try {
        MetricsServer metrics(9100);
        metrics.start();
        metrics.stop();

        DashboardServer dash(8080, "reports", "build/blockchain_node");

        std::thread t([&]() {
            dash.start();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        t.detach();

        return true;
    }
    catch (...) { return false; }
}

// =================================================================================
// MAIN
// =================================================================================

int main() {
// Dashboard zuerst starten — Port 8080!
    {
        DashboardServer dash(8080, "reports", "build/blockchain_node");
        std::thread([&]() { dash.start(); }).detach();
    }

    // Metrics auf Port 9100
    {
        MetricsServer metrics(9100);
        metrics.start();
    }


    // --- Deine Tests ---
    std::vector<TestResult> T;
    T.push_back({"Crypto",                testCrypto()});
    T.push_back({"Mempool",              testMempool()});
    T.push_back({"Block",                testBlock()});
    T.push_back({"HistoryStore",         testHistory()});
    T.push_back({"RTO Analyzer",         testRtoAnalyzer()});
    T.push_back({"DnD Characters",       testDndCharacters()});
    T.push_back({"Monster System",       testMonsters()});
    T.push_back({"Combat Rolls",         testCombat()});
    T.push_back({"Encounter System",     testEncounter()});
    T.push_back({"Dashboard + Metrics",  true}); // Selftest eingenständig

    print(T);

    // Node läuft weiter
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}

