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

static bool RUNNING = true;
void signalHandler(int) { RUNNING = false; }

// -------------------------------------------------------------
// Load config.json (safe)
// -------------------------------------------------------------
json loadConfig(const std::string& path)
{
    std::ifstream in(path);
    if (!in) {
        std::cerr << "[Config] Missing config.json\n";
        exit(1);
    }

    json j;
    in >> j;
    return j;
}

// -------------------------------------------------------------
// DM-KEY HELPER
// -------------------------------------------------------------
static constexpr std::size_t DM_PUBKEY_SIZE  = 32;
static constexpr std::size_t DM_PRIVKEY_SIZE = 64;

bool loadDmKeysFromConfig(const json& cfg,
                          std::vector<uint8_t>& dmPriv,
                          std::vector<uint8_t>& dmPub)
{
    if (!cfg.contains("dmPrivKey") || !cfg["dmPrivKey"].is_array() ||
        !cfg.contains("dmPubKey")  || !cfg["dmPubKey"].is_array()) {
        std::cerr << "[Config] dmPrivKey/dmPubKey not found or not arrays – falling back.\n";
        return false;
    }

    try {
        dmPriv = cfg["dmPrivKey"].get<std::vector<uint8_t>>();
        dmPub  = cfg["dmPubKey"].get<std::vector<uint8_t>>();
    }
    catch (const std::exception& ex) {
        std::cerr << "[Config] Failed to parse dmPrivKey/dmPubKey: "
                  << ex.what() << " – falling back.\n";
        dmPriv.clear();
        dmPub.clear();
        return false;
    }

    if (dmPriv.size() != DM_PRIVKEY_SIZE) {
        std::cerr << "[Config] Invalid dmPrivKey length: expected "
                  << DM_PRIVKEY_SIZE << " bytes, got " << dmPriv.size()
                  << " – falling back.\n";
        dmPriv.clear();
        dmPub.clear();
        return false;
    }

    if (dmPub.size() != DM_PUBKEY_SIZE) {
        std::cerr << "[Config] Invalid dmPubKey length: expected "
                  << DM_PUBKEY_SIZE << " bytes, got " << dmPub.size()
                  << " – falling back.\n";
        dmPriv.clear();
        dmPub.clear();
        return false;
    }

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

    // ---------------------------------------------------------
    // CONFIG
    // ---------------------------------------------------------
    json cfg = loadConfig("config.json");

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

    bool ok = loadDmKeysFromConfig(cfg, dmPriv, dmPub);

    if (!ok) {
        auto kp = crypto::generateKeyPair();
        dmPriv = kp.privateKey;
        dmPub  = kp.publicKey;
        std::cout << "[Config] Generated ephemeral DM keypair (fallback)\n";
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
    ctx.characterExists = [&](const std::string&) { return true; };
    ctx.monsterExists   = [&](const std::string&) { return true; };
    ctx.encounterActive = [&](const std::string&) { return true; };
    ctx.hasControlPermission = [&](const std::string&,
                                   const std::vector<uint8_t>&,
                                   bool) { return true; };

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
            std::string host = adr.value("host", "127.0.0.1");
            int         p    = adr.value("port", peerPort);
            peers.connectToPeer(host, p);
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

    ChainApi api(chain);
    api.bind(http);

    dnd::DndApi dndapi(chain, mempool, &peers, validator, dmPriv, dmPub);
    dndapi.install(http);

    DashboardServer dashboard(httpPort, "reports/", db);
    dashboard.attach(http);

    MetricsServer metrics(9100);
    metrics.attach(http);

    // ---------------------------------------------------------
    // HTTP SERVER THREAD (with error logging)
    // ---------------------------------------------------------
    std::thread thttp([&]() {
        std::cout << "[HTTP] Starting server on port " << httpPort << "...\n";

        bool ok = http.listen("0.0.0.0", httpPort);

        if (!ok) {
            std::cerr << "[HTTP] ERROR: Failed to start HTTP server on port "
                      << httpPort << " (listen() returned false)\n";
        } else {
            std::cout << "[HTTP] Listening on port " << httpPort << "\n";
        }
    });


    // ---------------------------------------------------------
    // AUTO-MINER
    // ---------------------------------------------------------
    std::thread miner([&]() {
        BlockBuilder builder(chain, dmPriv, dmPub);

        while (RUNNING) {
            std::this_thread::sleep_for(std::chrono::seconds(4));

            if (mempool.size() == 0)
                continue;

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
    while (RUNNING) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

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

