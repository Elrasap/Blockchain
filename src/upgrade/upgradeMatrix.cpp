#include "upgrade/upgrade_matrix.hpp"
#include <sstream>

void UpgradeMatrix::addResult(int from, int to, bool ok) {
    Row r;
    r.from = from;
    r.to = to;
    r.ok = ok;
    rows.push_back(r);
}

std::string UpgradeMatrix::renderTable() const {
    std::ostringstream out;
    out << "FROM | TO | STATUS\n";
    for (const auto& r : rows) {
        out << r.from << "    | "
            << r.to << "  | "
            << (r.ok ? "OK" : "FAIL")
            << "\n";
    }
    return out.str();
}

