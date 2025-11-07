#include "storage/snapshotManager.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

SnapshotManager::SnapshotManager(BlockStore* s)
    : store(s), snapshot_dir("./snapshots") {
    fs::create_directories(snapshot_dir);
}

void SnapshotManager::createSnapshot() {
    std::lock_guard<std::mutex> lock(mtx);
    auto blocks = store->loadAllBlocks();
    if (blocks.empty()) {
        std::cout << "[SnapshotManager] No blocks to snapshot.\n";
        return;
    }

    uint64_t height = blocks.back().header.height;
    std::string file = snapshot_dir + "/snapshot_" + std::to_string(height) + ".bin";
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "[SnapshotManager] Cannot open file for writing: " << file << std::endl;
        return;
    }

    for (auto& b : blocks) {
        auto data = b.serialize();
        uint64_t len = data.size();
        out.write(reinterpret_cast<char*>(&len), sizeof(len));
        out.write(reinterpret_cast<const char*>(data.data()), len);
    }

    out.close();
    std::cout << "[SnapshotManager] Snapshot written: " << file << std::endl;
}

bool SnapshotManager::restoreFromSnapshot() {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "[SnapshotManager] restoreFromSnapshot() called\n";

    std::string file = latestSnapshotFile();
    if (file.empty()) {
        std::cerr << "[SnapshotManager] No snapshot found.\n";
        return false;
    }

    std::ifstream in(file, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[SnapshotManager] Failed to open: " << file << std::endl;
        return false;
    }

    std::cout << "[SnapshotManager] Restoring from: " << file << std::endl;
    store->clear();

    size_t blockCount = 0;

    while (true) {
        if (in.peek() == EOF) break;          // nichts mehr da → raus
        uint64_t len = 0;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));

        if (in.eof() || !in.good()) break;
        if (len == 0 || len > 100'000'000) {
            std::cerr << "[SnapshotManager] Invalid len=" << len << "\n";
            break;
        }

        std::vector<uint8_t> buf(len);
        in.read(reinterpret_cast<char*>(buf.data()), len);
        if (in.gcount() != static_cast<std::streamsize>(len)) {
            std::cerr << "[SnapshotManager] Short read (" << in.gcount() << "/" << len << ")\n";
            break;
        }

        Block b;
        b.header.height = blockCount;
        store->appendBlock(b);
        ++blockCount;
    }

    in.close();
    std::cout << "[SnapshotManager] Restored " << blockCount << " placeholder blocks\n";
    std::cout << "[SnapshotManager] restoreFromSnapshot() finished\n";
    return true;
}



std::string SnapshotManager::latestSnapshotFile() const {
    std::lock_guard<std::mutex> lock(mtx);
    if (!fs::exists(snapshot_dir)) return "";
    uint64_t latest_height = 0;
    std::string latest_file;
    for (auto& entry : fs::directory_iterator(snapshot_dir)) {
        if (!entry.is_regular_file()) continue;
        auto name = entry.path().filename().string();
        if (name.rfind("snapshot_", 0) == 0) {
            try {
                uint64_t h = std::stoull(name.substr(9, name.find('.') - 9));
                if (h > latest_height) {
                    latest_height = h;
                    latest_file = entry.path().string();
                }
            } catch (...) {}
        }
    }
    return latest_file;
}

void SnapshotManager::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    if (!fs::exists(snapshot_dir)) return;
    for (auto& entry : fs::directory_iterator(snapshot_dir)) {
        if (entry.is_regular_file()) fs::remove(entry.path());
    }
    std::cout << "[SnapshotManager] Cleared all snapshots.\n";
}

