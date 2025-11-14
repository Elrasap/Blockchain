#include "storage/historyStore.hpp"
#include <sqlite3.h>
#include <iostream>
#include <sstream>
#include <ctime>

HistoryStore::HistoryStore(const std::string& dbPath) : path(dbPath) {}

bool HistoryStore::init() {
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) return false;
    const char* sql1 =
        "CREATE TABLE IF NOT EXISTS reliability_history ("
        "ts INTEGER PRIMARY KEY,"
        "integrity INTEGER,performance INTEGER,chaos INTEGER,forecast INTEGER,"
        "avg_rto REAL,pass_rate REAL,anomalies INTEGER);";
    const char* sql2 =
        "CREATE TABLE IF NOT EXISTS rto_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "filename TEXT,rto_ms REAL,restore_ms REAL,passed INTEGER,ts INTEGER DEFAULT(strftime('%s','now')));";
    char* err = nullptr;
    sqlite3_exec(db, sql1, nullptr, nullptr, &err);
    sqlite3_exec(db, sql2, nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
    sqlite3_close(db);
    return true;
}

bool HistoryStore::exec(const std::string& sql) {
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) return false;
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if (err) { std::cerr << "[SQLite] " << err << "\n"; sqlite3_free(err); }
    sqlite3_close(db);
    return rc == SQLITE_OK;
}

bool HistoryStore::insertReliability(const ReliabilityStatus& s) {
    std::ostringstream oss;
    oss << "INSERT INTO reliability_history "
           "(ts,integrity,performance,chaos,forecast,avg_rto,pass_rate,anomalies)"
           " VALUES(" << std::time(nullptr) << ","
        << (s.integrityOk?1:0) << "," << (s.perfOk?1:0) << ","
        << (s.chaosOk?1:0) << "," << (s.forecastOk?1:0) << ","
        << s.avgRto << "," << s.passRate << "," << s.anomalies << ");";
    return exec(oss.str());
}

bool HistoryStore::insertRtoRecords(const std::vector<RunMetrics>& runs) {
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) return false;
    const char* sql = "INSERT INTO rto_history(filename,rto_ms,restore_ms,passed) VALUES(?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    for (auto& r : runs) {
        sqlite3_bind_text(stmt, 1, r.filename.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 2, r.rto_ms);
        sqlite3_bind_double(stmt, 3, r.restore_ms);
        sqlite3_bind_int(stmt, 4, r.passed ? 1 : 0);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}

std::vector<RtoRecord> HistoryStore::loadRecentRto(int limit) {
    std::vector<RtoRecord> out;
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) return out;
    std::ostringstream oss;
    oss << "SELECT filename,rto_ms,restore_ms,passed FROM rto_history "
           "ORDER BY id DESC LIMIT " << limit << ";";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, oss.str().c_str(), -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RtoRecord r;
        r.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        r.rto_ms = sqlite3_column_double(stmt, 1);
        r.restore_ms = sqlite3_column_double(stmt, 2);
        r.passed = sqlite3_column_int(stmt, 3);
        out.push_back(r);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return out;
}

