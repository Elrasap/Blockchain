#pragma once
#include <cstdint>
#include <atomic>
#include <string>

namespace metrics {

class DndMetrics {
public:
    // Wird global verwendet
    static DndMetrics& instance();

    // Counters
    std::atomic<uint64_t> blocksTotal {0};
    std::atomic<uint64_t> dndTxsTotal {0};
    std::atomic<uint64_t> hpDamageTotal {0};

    // Render für Prometheus
    std::string renderPrometheus() const;

private:
    DndMetrics() = default;
};

} // namespace metrics

