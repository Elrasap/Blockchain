#pragma once
#include <string>
#include <vector>
#include "core/block.hpp"

class BlockStore {
public:
    explicit BlockStore(const std::string& path);
    bool appendBlock(const Block& block);
    std::vector<Block> loadAllBlocks() const;
    Block getLatestBlock() const;        // <-- hier ist die Deklaration
    void clear();
    void reset();


private:
    std::string file_path;
};

