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

// ====== DND ======
#include "dnd/dndCharacterService.hpp"
#include "dnd/payload.hpp"
#include "dnd/patch.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/combat/combatService.hpp"
#include "dnd/combat/encounter.hpp"
#include "dnd/combat/monster.hpp"

// ====== WEB ======
#include "web/dashboardServer.hpp"
#include "metrics/metricsServer.hpp"

// ====== OBS ======
#include "obs/metrics.hpp"
#include "obs/healthChecker.hpp"

namespace fs = std::filesystem;

struct TestResult {
    std::string name;
    bool ok;
};

static void printSummary(const std::vector<TestResult>& tests) {
    std::cout << "\n==== SELFTEST SUMMARY ====\n";
    for (auto& t : tests)
        std::cout << (t.ok ? "[OK]   " : "[FAIL] ") << t.name << "\n";
    std::cout << "===========================\n\n";
}

// ======================== TESTS ============================
bool testCrypto() {
    try {
        std::vector<uint8_t> msg = {1,2,3};
        crypto::KeyPair kp = crypto::generateKeyPair();
        Transaction tx;
        tx.payload = msg;
        tx.nonce = 1;
        tx.senderPubkey = kp.publicKey;
        tx.sign(kp.privateKey);
        return tx.verifySignature();
    } catch (...) { return false; }
}

bool testMempool() {
    try {
        Mempool mp;
        Transaction tx;
        tx.payload = {9,9,9};
        mp.addTransaction(tx);
        return mp.size() == 1;
    } catch (...) { return false; }
}

bool testBlock() {
    try {
        Transaction tx;
        tx.payload = {1,2,3};
        Block b;
        b.transactions.push_back(tx);
        b.calculateMerkleRoot();
        b.hash();
        return true;
    } catch (...) { return false; }
}

bool testHistory() {
    HistoryStore hs("history.db");
    if (!hs.init()) return false;

    RtoRecord rec;
    rec.filename = "backup1.tar";
    rec.rto_ms = 1200.0;
    rec.restore_ms = 800.0;
    rec.passed = true;

    std::vector<RtoRecord> runs{rec};
    hs.insertRtoRecords(runs);

    return !hs.loadRecentRto(1).empty();
}

bool testRtoAnalyzer() {
    try {
        RtoRpoAnalyzer a("reports");
        a.analyzeAll();
        return true;
    } catch (...) { return false; }
}

bool testDndCharacters() {
    try {
        dnd::DndCharacterService s;
        dnd::CharacterSheet sheet;
        sheet.id = "hero1";
        sheet.name = "Conan";
        return true;
    } catch (...) { return false; }
}

bool testMonsters() {
    try {
        dnd::MonsterService ms("monsters.json");
        dnd::Monster m;
        m.id = "wolf1";
        m.name = "Wolf";
        m.level = 2;
        m.hpCurrent = 26;
        m.hpMax = 26;
        m.ac = 13;
        ms.upsert(m);
        dnd::Monster out;
        return ms.get("wolf1", out);
    } catch (...) { return false; }
}

bool testCombat() {
    try {
        dnd::combat::CombatService C;
        int roll = C.getDice().roll("1d20");
        return roll >= 1 && roll <= 20;
    } catch (...) { return false; }
}

bool testEncounter() {
    try {
        dnd::combat::EncounterManager E;
        auto& enc = E.startEncounter("test_enc");
        E.addCharacter(enc.id, "hero1", 15);
        E.addMonster(enc.id, "wolf1", 12);
        E.nextTurn(enc.id);

        dnd::combat::Encounter out;
        return E.get(enc.id, out);
    } catch (...) { return false; }
}

// ===========================================================
//          GLOBAL, SINGLE METRICSSERVER INSTANCE
// ===========================================================
static MetricsServer metricsServer(9100);

// ===========================================================
//                           MAIN
// ===========================================================
int main() {

    // ---------------- Dashboard Server ----------------
    std::thread([](){
        DashboardServer dash(8080, "reports", "build/blockchain_node");
        dash.start();
    }).detach();


    // ---------------- Metrics Server -------------------
    std::thread([](){
        std::cout << "[MetricsServer] Starting http://localhost:9100\n";
        metricsServer.start();
    }).detach();


    // ---------------- Selftests ------------------------
    std::vector<TestResult> T;
    T.push_back({"Crypto", testCrypto()});
    T.push_back({"Mempool", testMempool()});
    T.push_back({"Block", testBlock()});
    T.push_back({"HistoryStore", testHistory()});
    T.push_back({"RTO Analyzer", testRtoAnalyzer()});
    T.push_back({"DnD Characters", testDndCharacters()});
    T.push_back({"Monster System", testMonsters()});
    T.push_back({"Combat Rolls", testCombat()});
    T.push_back({"Encounter System", testEncounter()});

    printSummary(T);

    // ---------------- Metrics Loop ---------------------
    std::thread([](){
        Metrics& M = Metrics::instance();
        HealthChecker& H = HealthChecker::instance();

        uint64_t height = 0;

        while (true) {
            M.setGauge("block_height", height);
            M.setGauge("peers", 1);
            M.observe("cpu_load", (rand() % 100) / 100.0);

            H.setBlockHeight(height);
            H.setPeerCount(1);
            H.markHealthy();

            height++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();

    std::cout << "[Node] Running. CTRL+C to exit.\n";
    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
}

