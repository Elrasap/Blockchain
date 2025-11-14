#pragma once
#include <string>
#include <vector>
#include "upgrade/schema_registry.hpp"

class UpgradeMatrix {
public:
    void addResult(int from, int to, bool ok);
    std::string renderTable() const;
private:
    struct Row { int from; int to; bool ok; };
    std::vector<Row> rows;
};


