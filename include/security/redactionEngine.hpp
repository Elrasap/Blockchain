#pragma once
#include <string>
#include <vector>
#include <memory>

class CryptoEngine {
public:
    void redactSnapshot(Snapshot& s, const RedactionPolicy& policy);
    void redactLogs(const std::string& logDir);
    void exportAnonymizedState();
};
