#include <iostream>
#include <thread>
#include <chrono>

#include "core/dmKeyManager.hpp"
#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"
#include "storage/blockStore.hpp"

#include "dnd/dndTxValidator.hpp"

#include "web/dashboardServer.hpp"
#include "metrics/metricsServer.hpp"

int main() {
    std::cout << "=== Blockchain Node Startup ===\n";

    // 1) Load / create DM keys
    DmKeyPair dmKeys;
    if (!loadOrCreateDmKey("dm_keys.bin", dmKeys)) {
        std::cerr << "[FATAL] Cannot load or generate DM key\n";
        return 1;
    }

    // 2) BlockStore + Blockchain
    BlockStore store("blocks.db");
    Blockchain chain(store, dmKeys.publicKey);

    chain.ensureGenesisBlock(dmKeys.privateKey);

    // 3) Build DnD validation context
    dnd::DndValidationContext ctx;

    ctx.characterExists = [&chain](const std::string& id) {
        return chain.getDndState().characterExists(id);
    };
    ctx.monsterExists = [&chain](const std::string& id) {
        return chain.getDndState().monsterExists(id);
    };
    ctx.encounterActive = [&chain](const std::string& id) {
        return chain.getDndState().encounterExists(id);
    };

    ctx.hasControlPermission = [](const std::string&,
                                  const std::vector<uint8_t>&,
                                  bool) {
        return true; // TODO real auth
    };

    dnd::DndTxValidator validator(ctx);

    // 4) Mempool
    Mempool mempool;

    // 5) BlockBuilder
    BlockBuilder blockBuilder(chain,
                              dmKeys.privateKey,
                              dmKeys.publicKey);

    // 6) Dashboard Server
    DashboardServer dashboard(8080, "reports", "./blockchain_node");

    std::thread dashThread([&]() {
        dashboard.start();
    });

    // 7) Metrics
    MetricsServer metrics(9100);

    std::thread metricsThread([&]() {
        metrics.start();
    });

    // 8) Auto miner
    std::thread miner([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(4));

            if (mempool.size() == 0)
                continue;

            Block out;
            if (blockBuilder.buildAndAppendFromMempool(mempool, out)) {
                std::cout << "[AutoMiner] Block " << out.header.height << " mined.\n";
            }
        }
    });

    dashThread.join();
    metricsThread.join();
    miner.join();

    return 0;
}

