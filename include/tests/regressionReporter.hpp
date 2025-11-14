#pragma once
#include <string>
#include <vector>

struct RegressionResult {
    std::string version;
    bool passed;
    std::string diff;
};

class RegressionReporter {
public:
    void add(const RegressionResult& r);
    std::string toMarkdown() const;
    std::string toJson() const;
private:
    std::vector<RegressionResult> results;
};

