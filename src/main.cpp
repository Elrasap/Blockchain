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
#include "core/crypto.hpp"   // <--- IMPORTANT

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
// MAIN
// -------------------------------------------------------------
int main()
{
    std::cout << "=== DND BLOCKCHAIN NODE STARTING ===\n";

    // Shutdown signal
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
    // DM KEYS: from config OR auto-generate
    // ---------------------------------------------------------
    std::vector<uint8_t> dmPriv;
    std::vector<uint8_t> dmPub;

    if (cfg.contains("dmPrivKey") && cfg["dmPrivKey"].is_array() &&
        cfg.contains("dmPubKey")  && cfg["dmPubKey"].is_array())
    {
        dmPriv = cfg["dmPrivKey"].get<std::vector<uint8_t>>();
        dmPub  = cfg["dmPubKey"].get<std::vector<uint8_t>>();
        std::cout << "[Config] Using DM keys from config.json\n";
    }
    else {
        auto kp = crypto::generateKeyPair();
        dmPriv = kp.privateKey;
        dmPub  = kp.publicKey;
        std::cout << "[Config] Generated ephemeral DM keypair (no keys in config.json)\n";
    }

    // ---------------------------------------------------------
    // STORAGE
    // ---------------------------------------------------------
    BlockStore store(db);

    // ---------------------------------------------------------
    // BLOCKCHAIN
    // ---------------------------------------------------------
    Blockchain chain(store, dmPub);

    // Load snapshot & rebuild state
    chain.loadSnapshot(snap);
    chain.rebuildState();

    chain.ensureGenesisBlock(dmPriv);

    // ---------------------------------------------------------
    // VALIDATOR (DND)
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

    // Safe peer loading
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

    std::thread thttp([&]() {
        http.listen("0.0.0.0", httpPort);
    });

    // ---------------------------------------------------------
    // AUTO-MINER (every 4 seconds)
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

