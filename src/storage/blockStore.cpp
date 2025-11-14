#include "storage/blockStore.hpp"
#include <fstream>
#include <cstring>
#include <cstdio>   // für std::remove

using namespace std;

BlockStore::BlockStore(const std::string& path)
    : file_path(path) {}

bool BlockStore::appendBlock(const Block& block) {
    std::ofstream out(file_path, std::ios::app | std::ios::binary);
    if (!out.is_open()) return false;

    std::vector<uint8_t> data = block.serialize();
    uint64_t size = data.size();

    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    out.write(reinterpret_cast<const char*>(data.data()), size);
    return true;
}

std::vector<Block> BlockStore::loadAllBlocks() const {
    std::vector<Block> blocks;
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) return blocks;

    const size_t header_size = 32 + 32 + sizeof(uint64_t) * 3;

    while (true) {
        uint64_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in || size == 0) break;

        std::vector<uint8_t> data(size);
        in.read(reinterpret_cast<char*>(data.data()), size);
        if (in.gcount() < static_cast<std::streamsize>(size)) break;
        if (data.size() < header_size) break;

        Block b;
        size_t offset = 0;

        std::memcpy(b.header.prevHash.data(), data.data() + offset, 32); offset += 32;
        std::memcpy(b.header.merkleRoot.data(), data.data() + offset, 32); offset += 32;

        std::memcpy(&b.header.height,    data.data() + offset, sizeof(uint64_t)); offset += sizeof(uint64_t);
        std::memcpy(&b.header.timestamp, data.data() + offset, sizeof(uint64_t)); offset += sizeof(uint64_t);
        std::memcpy(&b.header.nonce,     data.data() + offset, sizeof(uint64_t)); offset += sizeof(uint64_t);

        blocks.push_back(b);
    }

    return blocks;
}

Block BlockStore::getLatestBlock() const {
    std::vector<Block> all = loadAllBlocks();
    if (all.empty()) return Block();
    return all.back();
}

void BlockStore::clear() {
    std::ofstream out(file_path, std::ios::trunc);
    out.close();
}

void BlockStore::reset() {
    std::remove(file_path.c_str());
}

