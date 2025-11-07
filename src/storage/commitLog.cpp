#include "storage/commitLog.hpp"
#include "core/crypto.hpp"
#include "consensus/consensusManager.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

using namespace std;

CommitLog::CommitLog(const string& path) : filePath(path) {
    // Stelle sicher, dass das Verzeichnis existiert
    filesystem::path p(path);
    if (p.has_parent_path())
        filesystem::create_directories(p.parent_path());
}

void CommitLog::append(const CommitEntry& entry) {
    lock_guard<mutex> lock(mtx);
    ofstream out(filePath, ios::app);
    if (!out.is_open()) return;
    out << entry.height << " " << toHex(entry.hash) << " "
        << (entry.status == CommitStatus::Committed ? "committed" : "pending") << "\n";
}

void CommitLog::markCommitted(const array<uint8_t, 32>& hash) {
    lock_guard<mutex> lock(mtx);
    ifstream in(filePath);
    if (!in.is_open()) return;

    vector<string> lines;
    string line;
    string hashHex = toHex(hash);

    while (getline(in, line)) {
        istringstream iss(line);
        uint64_t height;
        string h, status;
        iss >> height >> h >> status;
        if (h == hashHex) line = to_string(height) + " " + h + " committed";
        lines.push_back(line);
    }
    in.close();

    ofstream out(filePath, ios::trunc);
    for (const auto& l : lines)
        out << l << "\n";
}

CommitEntry CommitLog::getLatest() const {
    lock_guard<mutex> lock(mtx);
    CommitEntry latest{};
    ifstream in(filePath);
    if (!in.is_open()) return latest;

    string line, last;
    while (getline(in, line)) last = line;
    if (last.empty()) return latest;

    istringstream iss(last);
    uint64_t height;
    string hashHex, status;
    iss >> height >> hashHex >> status;

    latest.height = height;
    latest.status = (status == "committed" ? CommitStatus::Committed : CommitStatus::Pending);

    // Defensive parsing (falls Hash zu kurz ist)
    for (size_t i = 0; i < 32 && i * 2 + 1 < hashHex.size(); ++i)
        latest.hash[i] = static_cast<uint8_t>(stoul(hashHex.substr(i * 2, 2), nullptr, 16));

    return latest;
}

vector<CommitEntry> CommitLog::loadAll() const {
    lock_guard<mutex> lock(mtx);
    vector<CommitEntry> entries;
    ifstream in(filePath);
    if (!in.is_open()) return entries;

    string line;
    while (getline(in, line)) {
        istringstream iss(line);
        uint64_t height;
        string hashHex, status;
        iss >> height >> hashHex >> status;

        if (hashHex.size() < 64) continue; // Überspringe unvollständige Zeilen

        CommitEntry e{};
        e.height = height;
        for (size_t i = 0; i < 32 && i * 2 + 1 < hashHex.size(); ++i)
            e.hash[i] = static_cast<uint8_t>(stoul(hashHex.substr(i * 2, 2), nullptr, 16));

        e.status = (status == "committed" ? CommitStatus::Committed : CommitStatus::Pending);
        entries.push_back(e);
    }
    return entries;
}

void CommitLog::clear() {
    lock_guard<mutex> lock(mtx);
    ofstream out(filePath, ios::trunc);
}

