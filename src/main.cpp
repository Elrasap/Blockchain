#include <array>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/blockBuilder.hpp"
#include "core/blockchain.hpp"
#include "core/dmKeyManager.hpp"
#include "core/mempool.hpp"
#include "metrics/metricsServer.hpp"
#include "network/gossipServer.hpp"
#include "network/peerManager.hpp"
#include "network/syncManager.hpp"
#include "storage/blockStore.hpp"
#include "web/chainApi.hpp"
#include "web/dashboardServer.hpp"
#include "web/dndApi.hpp"

using json = nlohmann::json;

namespace {

volatile std::sig_atomic_t running = 1;

void signalHandler(int)
{
    running = 0;
}

json loadConfig(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        std::cout << "[Config] " << path
                  << " not found, using local defaults\n";
        return json::object();
    }

    json config = json::parse(input, nullptr, false);
    if (config.is_discarded() || !config.is_object())
        throw std::runtime_error("invalid config.json");
    if (config.contains("dmPrivKey"))
        throw std::runtime_error(
            "dmPrivKey is no longer accepted in config.json; use dmKeyFile");
    return config;
}

} // namespace

int main()
{
    std::cout << "=== DND BLOCKCHAIN NODE STARTING ===\n";
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    json config;
    try {
        config = loadConfig("config.json");
    } catch (const std::exception& ex) {
        std::cerr << "[Config] " << ex.what() << "\n";
        return 1;
    }

    const int httpPort = config.value("port", 8080);
    const int gossipPort = config.value("gossipPort", 8090);
    const int peerPort = config.value("peerPort", 9000);
    const std::string blockDirectory = config.value("blockDb", "blocks");
    const std::string mempoolFile = config.value("mempoolFile", "mempool.json");
    const std::string snapshotFile = config.value("snapshotFile", "state_snapshot.json");
    const std::string dmKeyFile = config.value("dmKeyFile", "keys/dm.key");

    DmKeyPair dmKeys;
    if (!loadOrCreateDmKey(dmKeyFile, dmKeys)) {
        std::cerr << "[Node] Could not load the DM key\n";
        return 1;
    }

    BlockStore store(blockDirectory);
    Blockchain chain(store, dmKeys.publicKey);
    if (!chain.ensureGenesisBlock(dmKeys.privateKey)) {
        std::cerr << "[Node] Could not initialize the blockchain\n";
        return 1;
    }

    Mempool mempool(
        chain.transactionValidator(),
        [&chain]() { return chain.getDndState(); },
        [&chain](const std::array<uint8_t, 32>& hash) {
            return chain.hasTransaction(hash);
        });
    mempool.loadFromFile(mempoolFile);

    PeerManager peers(peerPort);
    SyncManager sync(chain, peers, &mempool);
    peers.setSync(&sync);
    peers.setMempool(&mempool);
    peers.startServer();
    sync.start();

    if (config.contains("peers") && config["peers"].is_array()) {
        for (const auto& address : config["peers"]) {
            peers.connectToPeer(address.value("host", "127.0.0.1"),
                                address.value("port", peerPort));
        }
    }

    GossipServer gossip(gossipPort, chain, mempool, &peers);
    std::thread gossipThread([&]() { gossip.start(); });

    httplib::Server http;
    auto chainApi = std::make_shared<ChainApi>(chain, &peers);
    chainApi->bind(http);

    auto dndApi = std::make_shared<dnd::DndApi>(
        chain, mempool, &peers, dmKeys.privateKey, dmKeys.publicKey);
    dndApi->install(http);

    auto dashboard = std::make_shared<DashboardServer>(
        httpPort, "reports/", blockDirectory);
    dashboard->attach(http);

    MetricsServer metrics(9100);
    metrics.attach(http);

    std::thread httpThread([&]() {
        std::cout << "[HTTP] Starting server on port " << httpPort << "...\n";
        http.listen("0.0.0.0", httpPort);
    });

    std::thread miner([&]() {
        BlockBuilder builder(chain, dmKeys.privateKey, dmKeys.publicKey);
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(4));
            if (!running || mempool.size() == 0) continue;

            Block block;
            if (builder.buildAndAppendFromMempool(mempool, block)) {
                std::cout << "[Miner] Mined block height "
                          << block.header.height << "\n";
                peers.broadcastBlock(block);
            }
        }
    });

    while (running)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "[Node] Shutting down...\n";
    http.stop();
    gossip.stop();
    sync.stop();
    peers.stop();

    if (miner.joinable()) miner.join();
    if (gossipThread.joinable()) gossipThread.join();
    if (httpThread.joinable()) httpThread.join();

    mempool.saveToFile(mempoolFile);
    chain.writeSnapshot(snapshotFile);
    return 0;
}
