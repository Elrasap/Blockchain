#include "tests/regressionReporter.hpp"
#include <sstream>

void RegressionReporter::add(const RegressionResult& r) {
    results.push_back(r);
}

std::string RegressionReporter::toMarkdown() const {
    std::stringstream ss;
    ss << "| Version | Status | Details |\n";
    ss << "|---------|--------|---------|\n";
    for (const auto& r : results) {
        ss << "| "
           << r.version << " | "
           << (r.passed ? "PASS" : "FAIL") << " | "
           << (r.diff.empty() ? "-" : r.diff)
           << " |\n";
    }
    return ss.str();
}

std::string RegressionReporter::toJson() const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        ss << "{"
           << "\"version\":\"" << r.version << "\","
           << "\"passed\":" << (r.passed ? "true" : "false") << ","
           << "\"diff\":\"" << r.diff << "\""
           << "}";
        if (i + 1 < results.size()) ss << ",";
    }
    ss << "]";
    return ss.str();
}

