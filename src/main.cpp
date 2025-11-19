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

#include "network/gossipServer.hpp"
#include "network/peerManager.hpp"

int main() {
    std::cout << "=== Blockchain Node Startup ===\n";

    // 1) Load DM key
    DmKeyPair dmKeys;
    if (!loadOrCreateDmKey("dm_keys.bin", dmKeys)) {
        std::cerr << "[FATAL] Cannot load or generate DM key\n";
        return 1;
    }

    // 2) Block store + chain
    BlockStore store("blocks.db");
    Blockchain chain(store, dmKeys.publicKey);
    chain.ensureGenesisBlock(dmKeys.privateKey);

    // 3) DnD validation context
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
        return true;
    };

    dnd::DndTxValidator validator(ctx);

    // 4) Mempool (mit Validator!)
    Mempool mempool(&validator);

    // 5) BlockBuilder
    BlockBuilder blockBuilder(chain,
                              dmKeys.privateKey,
                              dmKeys.publicKey);

    // 6) PeerManager
    PeerManager peers(8091, nullptr);
    peers.setMempool(&mempool);
    peers.startServer();

    // 7) Dashboard
    DashboardServer dashboard(8080, "reports", "./blockchain_node");
    std::thread dashThread([&]() {
        dashboard.start();
    });

    // 8) Metrics
    MetricsServer metrics(9100);
    std::thread metricsThread([&]() {
        metrics.start();
    });

    // 9) Gossip Server
    GossipServer gossip(8090, chain, mempool, &peers, &validator);
    std::thread gossipThread([&]() {
        gossip.start();
    });

    // 10) Auto miner
    std::thread miner([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(4));

            if (mempool.size() == 0)
                continue;

            Block out;
            if (blockBuilder.buildAndAppendFromMempool(mempool, out)) {
                std::cout << "[AutoMiner] Block "
                          << out.header.height << " mined.\n";

                if (peers.peerCount() > 0) {
                    peers.broadcastBlock(out);
                }
            }
        }
    });

    dashThread.join();
    metricsThread.join();
    gossipThread.join();
    miner.join();
}

