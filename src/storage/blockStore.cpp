#include "storage/blockStore.hpp"

#include <sqlite3.h>
#include <stdexcept>
#include <iostream>
#include <cstdio>     // std::remove
#include <vector>
#include <cstring>

using namespace std;

BlockStore::BlockStore(const std::string& path)
    : db_path(path), db(nullptr) {

    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::string msg = "Failed to open blocks DB: ";
        msg += sqlite3_errmsg(db);
        sqlite3_close(db);
        db = nullptr;
        throw std::runtime_error(msg);
    }

    if (!initSchema()) {
        throw std::runtime_error("Failed to init blocks schema");
    }
}

BlockStore::~BlockStore() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool BlockStore::initSchema() {
    if (!db) return false;

    const char* sql =
        "CREATE TABLE IF NOT EXISTS blocks ("
        " height INTEGER PRIMARY KEY,"
        " timestamp INTEGER NOT NULL,"
        " prev_hash BLOB NOT NULL,"
        " merkle_root BLOB NOT NULL,"
        " validator_pubkey BLOB NOT NULL,"
        " signature BLOB NOT NULL,"
        " block_hash BLOB NOT NULL"
        ");";

    char* err = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "BlockStore schema error: " << (err ? err : "") << "\n";
        if (err) sqlite3_free(err);
        return false;
    }
    if (err) sqlite3_free(err);
    return true;
}

bool BlockStore::appendBlock(const Block& block) {
    if (!db) return false;

    const char* sql =
        "INSERT OR REPLACE INTO blocks("
        " height, timestamp, prev_hash, merkle_root,"
        " validator_pubkey, signature, block_hash"
        ") VALUES (?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "appendBlock prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    const BlockHeader& h = block.header;
    auto hsh = block.hash();

    rc = sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(h.height));
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(h.timestamp));

    if (rc == SQLITE_OK)
        rc = sqlite3_bind_blob(stmt, 3,
                               h.prevHash.data(),
                               static_cast<int>(h.prevHash.size()),
                               SQLITE_STATIC);

    if (rc == SQLITE_OK)
        rc = sqlite3_bind_blob(stmt, 4,
                               h.merkleRoot.data(),
                               static_cast<int>(h.merkleRoot.size()),
                               SQLITE_STATIC);

    if (rc == SQLITE_OK)
        rc = sqlite3_bind_blob(stmt, 5,
                               h.validatorPubKey.data(),
                               static_cast<int>(h.validatorPubKey.size()),
                               SQLITE_STATIC);

    if (rc == SQLITE_OK)
        rc = sqlite3_bind_blob(stmt, 6,
                               h.signature.data(),
                               static_cast<int>(h.signature.size()),
                               SQLITE_STATIC);

    if (rc == SQLITE_OK)
        rc = sqlite3_bind_blob(stmt, 7,
                               hsh.data(),
                               static_cast<int>(hsh.size()),
                               SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        std::cerr << "appendBlock bind failed: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    rc = sqlite3_step(stmt);
    bool ok = (rc == SQLITE_DONE);
    if (!ok) {
        std::cerr << "appendBlock step failed: " << sqlite3_errmsg(db) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}

std::vector<Block> BlockStore::loadAllBlocks() const {
    std::vector<Block> blocks;
    if (!db) return blocks;

    const char* sql =
        "SELECT height, timestamp, prev_hash, merkle_root,"
        "       validator_pubkey, signature"
        "  FROM blocks"
        " ORDER BY height ASC;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "loadAllBlocks prepare failed: " << sqlite3_errmsg(db) << "\n";
        return blocks;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Block b;

        b.header.height = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        b.header.timestamp = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));

        // prev_hash
        {
            const void* blob = sqlite3_column_blob(stmt, 2);
            int len = sqlite3_column_bytes(stmt, 2);
            if (blob && len == 32) {
                std::memcpy(b.header.prevHash.data(), blob, 32);
            }
        }

        // merkle_root
        {
            const void* blob = sqlite3_column_blob(stmt, 3);
            int len = sqlite3_column_bytes(stmt, 3);
            if (blob && len == 32) {
                std::memcpy(b.header.merkleRoot.data(), blob, 32);
            }
        }

        // validator_pubkey
        {
            const void* blob = sqlite3_column_blob(stmt, 4);
            int len = sqlite3_column_bytes(stmt, 4);
            if (blob && len > 0) {
                const uint8_t* ptr = static_cast<const uint8_t*>(blob);
                b.header.validatorPubKey.assign(ptr, ptr + len);
            }
        }

        // signature
        {
            const void* blob = sqlite3_column_blob(stmt, 5);
            int len = sqlite3_column_bytes(stmt, 5);
            if (blob && len > 0) {
                const uint8_t* ptr = static_cast<const uint8_t*>(blob);
                b.header.signature.assign(ptr, ptr + len);
            }
        }

        blocks.push_back(std::move(b));
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "loadAllBlocks step failed: " << sqlite3_errmsg(db) << "\n";
    }

    sqlite3_finalize(stmt);
    return blocks;
}

Block BlockStore::getLatestBlock() const {
    if (!db) return Block{};

    const char* sql =
        "SELECT height, timestamp, prev_hash, merkle_root,"
        "       validator_pubkey, signature"
        "  FROM blocks"
        " ORDER BY height DESC"
        " LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "getLatestBlock prepare failed: " << sqlite3_errmsg(db) << "\n";
        return Block{};
    }

    Block b;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        b.header.height = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        b.header.timestamp = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));

        // prev_hash
        {
            const void* blob = sqlite3_column_blob(stmt, 2);
            int len = sqlite3_column_bytes(stmt, 2);
            if (blob && len == 32) {
                std::memcpy(b.header.prevHash.data(), blob, 32);
            }
        }

        // merkle_root
        {
            const void* blob = sqlite3_column_blob(stmt, 3);
            int len = sqlite3_column_bytes(stmt, 3);
            if (blob && len == 32) {
                std::memcpy(b.header.merkleRoot.data(), blob, 32);
            }
        }

        // validator_pubkey
        {
            const void* blob = sqlite3_column_blob(stmt, 4);
            int len = sqlite3_column_bytes(stmt, 4);
            if (blob && len > 0) {
                const uint8_t* ptr = static_cast<const uint8_t*>(blob);
                b.header.validatorPubKey.assign(ptr, ptr + len);
            }
        }

        // signature
        {
            const void* blob = sqlite3_column_blob(stmt, 5);
            int len = sqlite3_column_bytes(stmt, 5);
            if (blob && len > 0) {
                const uint8_t* ptr = static_cast<const uint8_t*>(blob);
                b.header.signature.assign(ptr, ptr + len);
            }
        }
    } else {
        // keine Zeilen -> leere Chain
        b = Block{};
    }

    sqlite3_finalize(stmt);
    return b;
}

void BlockStore::clear() {
    if (!db) return;

    const char* sql = "DELETE FROM blocks;";
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "BlockStore clear error: " << (err ? err : "") << "\n";
        if (err) sqlite3_free(err);
    }
    if (err) sqlite3_free(err);
}

void BlockStore::reset() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }

    std::remove(db_path.c_str());

    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "BlockStore reset: failed to reopen DB: "
                  << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        db = nullptr;
        return;
    }

    if (!initSchema()) {
        std::cerr << "BlockStore reset: failed to init schema\n";
    }
}

