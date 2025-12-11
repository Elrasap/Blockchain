#pragma once
#include <cstdint>
#include <atomic>
#include <string>

namespace metrics {

class DndMetrics {
public:

    static DndMetrics& instance();


    std::atomic<uint64_t> blocksTotal {0};
    std::atomic<uint64_t> dndTxsTotal {0};
    std::atomic<uint64_t> hpDamageTotal {0};


    std::string renderPrometheus() const;

private:
    DndMetrics() = default;
};

}

