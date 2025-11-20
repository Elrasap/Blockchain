#include <iostream>
#include <fstream>
#include <thread>
#include <csignal>
#include <chrono>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Core
#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"
#include "core/crypto.hpp"

// DnD
#include "dnd/dndTxValidator.hpp"
#include "dnd/dndPayload.hpp"
#include "dnd/dndState.hpp"

// Web & Network
#include "web/chainApi.hpp"
#include "web/dndApi.hpp"
#include "web/dashboardServer.hpp"
#include "metrics/metricsServer.hpp"
#include "network/gossipServer.hpp"
#include "network/peerManager.hpp"
#include "network/syncManager.hpp"

// Storage
#include "storage/blockStore.hpp"


// -----------------------------
// Runtime flag
// -----------------------------
static bool RUNNING = true;
void signalHandler(int) { RUNNING = false; }


// -------------------------------------------------------------
// Load config.json
// -------------------------------------------------------------
json loadConfig(const std::string& path)
{
    std::ifstream in(path);
    if (!in) {
        std::cerr << "[Config] Missing " << path << "\n";
        std::exit(1);
    }
    json j;
    in >> j;
    return j;
}

// -------------------------------------------------------------
// Save config.json
// -------------------------------------------------------------
bool saveConfig(const std::string& path, const json& cfg)
{
    std::ofstream out(path);
    if (!out) return false;
    out << cfg.dump(4) << "\n";
    return true;
}

// -------------------------------------------------------------
// DM KEY helper
// -------------------------------------------------------------
static constexpr std::size_t DM_PUBKEY_SIZE  = 32;
static constexpr std::size_t DM_PRIVKEY_SIZE = 64;

bool loadDmKeysFromConfig(const json& cfg,
                          std::vector<uint8_t>& dmPriv,
                          std::vector<uint8_t>& dmPub)
{
    if (!cfg.contains("dmPrivKey") || !cfg.contains("dmPubKey"))
        return false;

    try {
        dmPriv = cfg["dmPrivKey"].get<std::vector<uint8_t>>();
        dmPub  = cfg["dmPubKey"].get<std::vector<uint8_t>>();
    } catch (...) {
        return false;
    }

    if (dmPriv.size() != DM_PRIVKEY_SIZE) return false;
    if (dmPub.size()  != DM_PUBKEY_SIZE)  return false;

    std::cout << "[Config] Using DM keys from config.json\n";
    return true;
}


// -------------------------------------------------------------
// MAIN
// -------------------------------------------------------------
int main()
{
    std::cout << "=== DND BLOCKCHAIN NODE STARTING ===\n";

    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    const std::string configPath = "config.json";

    // ---------------------------------------------------------
    // CONFIG
    // ---------------------------------------------------------
    json cfg = loadConfig(configPath);

    int httpPort   = cfg.value("port",       8080);
    int gossipPort = cfg.value("gossipPort", 8090);
    int peerPort   = cfg.value("peerPort",   9000);

    std::string db   = cfg.value("blockDb",      "blocks.db");
    std::string mpf  = cfg.value("mempoolFile",  "mempool.json");
    std::string snap = cfg.value("snapshotFile", "state_snapshot.json");

    // ---------------------------------------------------------
    // DM KEYS
    // ---------------------------------------------------------
    std::vector<uint8_t> dmPriv;
    std::vector<uint8_t> dmPub;

    if (!loadDmKeysFromConfig(cfg, dmPriv, dmPub)) {
        std::cout << "[Config] No DM keys found, generating...\n";

        crypto::KeyPair kp = crypto::generateKeyPair();
        dmPriv = kp.privateKey;
        dmPub  = kp.publicKey;

        cfg["dmPrivKey"] = dmPriv;
        cfg["dmPubKey"]  = dmPub;

        saveConfig(configPath, cfg);
        std::cout << "[Config] New DM keys stored.\n";
    }

    // ---------------------------------------------------------
    // STORAGE
    // ---------------------------------------------------------
    BlockStore store(db);

    // ---------------------------------------------------------
    // BLOCKCHAIN
    // ---------------------------------------------------------
    Blockchain chain(store, dmPub);

    chain.loadSnapshot(snap);
    chain.rebuildState();
    chain.ensureGenesisBlock(dmPriv);

    // ---------------------------------------------------------
    // VALIDATOR
    // ---------------------------------------------------------
    dnd::DndValidationContext ctx;
    ctx.characterExists       = [&](const std::string&) { return true; };
    ctx.monsterExists         = [&](const std::string&) { return true; };
    ctx.encounterActive       = [&](const std::string&) { return true; };
    ctx.hasControlPermission  =
        [&](const std::string&, const std::vector<uint8_t>&, bool) { return true; };

    dnd::DndTxValidator validator(ctx);

    // ---------------------------------------------------------
    // MEMPOOL
    // ---------------------------------------------------------
    Mempool mempool(&validator);
    mempool.loadFromFile(mpf);

    // ---------------------------------------------------------
    // NETWORK
    // ---------------------------------------------------------
    PeerManager peers(peerPort);
    SyncManager sync(chain, peers);
    peers.setSync(&sync);
    global_sync = &sync;

    peers.startServer();

    if (cfg.contains("peers") && cfg["peers"].is_array()) {
        for (auto& adr : cfg["peers"]) {
            peers.connectToPeer(
                adr.value("host", "127.0.0.1"),
                adr.value("port", peerPort)
            );
        }
    }

    // ---------------------------------------------------------
    // GOSSIP SERVER
    // ---------------------------------------------------------
    GossipServer gossip(gossipPort, chain, mempool, &peers, &validator);
    std::thread tg([&]() { gossip.start(); });

    // ---------------------------------------------------------
    // HTTP API
    // ---------------------------------------------------------
    httplib::Server http;

    auto chainApi = std::make_shared<ChainApi>(chain, &peers);
    chainApi->bind(http);

    auto dndapi = std::make_shared<dnd::DndApi>(
        chain, mempool, &peers, validator, dmPriv, dmPub
    );
    dndapi->install(http);

    auto dashboard = std::make_shared<DashboardServer>(httpPort, "reports/", db);
    dashboard->attach(http);

    MetricsServer metrics(9100);
    metrics.attach(http);

    // ---------------------------------------------------------
    // HTTP SERVER THREAD
    // ---------------------------------------------------------
    std::thread thttp([&]() {
        std::cout << "[HTTP] Starting server on port " << httpPort << "...\n";
        http.listen("0.0.0.0", httpPort);
    });

    // ---------------------------------------------------------
    // AUTO-MINER
    // ---------------------------------------------------------
    std::thread miner([&]() {
        BlockBuilder builder(chain, dmPriv, dmPub);

        while (RUNNING) {
            std::this_thread::sleep_for(std::chrono::seconds(4));

            if (mempool.size() == 0) continue;

            Block out;

            if (builder.buildAndAppendFromMempool(mempool, out)) {
                std::cout << "[Miner] Mined block height "
                          << out.header.height << "\n";

                peers.broadcastBlock(out);
            }
        }
    });

    // ---------------------------------------------------------
    // MAIN LOOP
    // ---------------------------------------------------------
    while (RUNNING)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // ---------------------------------------------------------
    // CLEAN SHUTDOWN
    // ---------------------------------------------------------
    std::cout << "[Node] Shutting down...\n";

    mempool.saveToFile(mpf);
    chain.writeSnapshot(snap);

    peers.stop();
    http.stop();
    gossip.stop();

    if (miner.joinable()) miner.join();
    if (tg.joinable())    tg.join();
    if (thttp.joinable()) thttp.join();

    return 0;
}

