#include "chaos/chaos_engine.hpp"
#include <algorithm>

ChaosEngine::ChaosEngine(int clusterSize) : size(clusterSize) {
    alive.assign(size, true);
}

void ChaosEngine::reset() {
    std::fill(alive.begin(), alive.end(), true);
    baseLatencyMs = 250.0;
    lossPct = 0.0;
    cpuLoadPct = 5.0;
    diskError = false;
    peers = std::max(1, size - 1);
    sinceLastCommit = std::chrono::milliseconds(0);
    timeToRecover = std::chrono::milliseconds(0);
    finalityLag = std::chrono::milliseconds(0);
    crashCount = 0;
    peerDrops = 0;
    maxLatency = 0.0;
}

void ChaosEngine::applyFault(const ChaosFault& f) {
    switch (f.type) {
        case FaultType::Crash: {
            int idx = (f.targetNode < 0) ? 0 : f.targetNode % size;
            if (alive[idx]) { alive[idx] = false; crashCount++; peers = std::max(0, peers - 1); peerDrops++; }
            timeToRecover += f.duration;
            break;
        }
        case FaultType::LatencySpike: {
            baseLatencyMs += f.intensity;
            maxLatency = std::max(maxLatency, baseLatencyMs);
            finalityLag += f.duration;
            break;
        }
        case FaultType::PacketLoss: {
            lossPct = std::min(100.0, lossPct + f.intensity);
            if (f.intensity > 10.0) { peerDrops += 1; finalityLag += f.duration; }
            break;
        }
        case FaultType::CpuSpike: {
            cpuLoadPct = std::min(100.0, cpuLoadPct + f.intensity);
            finalityLag += std::chrono::milliseconds((int)(f.duration.count() * 0.5));
            break;
        }
        case FaultType::DiskError: {
            diskError = true;
            finalityLag += f.duration;
            break;
        }
    }
}

void ChaosEngine::tick(std::chrono::milliseconds dt) {
    sinceLastCommit += dt;
    // Commit „Erfolg“: wenn mind. 2 Nodes leben und loss/latency moderat
    int aliveCount = 0; for (bool a : alive) if (a) aliveCount++;
    bool canCommit = (aliveCount >= 2) && (lossPct < 50.0) && (baseLatencyMs < 1500.0) && !diskError;
    if (canCommit && sinceLastCommit >= std::chrono::milliseconds(500)) {
        sinceLastCommit = std::chrono::milliseconds(0);
        if (timeToRecover.count() > 0) {
            auto step = std::min<std::int64_t>(dt.count(), timeToRecover.count());
            timeToRecover -= std::chrono::milliseconds(step);
        }
        if (finalityLag.count() > 0) {
            auto step = std::min<std::int64_t>(dt.count(), finalityLag.count());
            finalityLag -= std::chrono::milliseconds(step);
        }
    }
    maxLatency = std::max(maxLatency, baseLatencyMs);
}

ChaosMetrics ChaosEngine::snapshot() const {
    ChaosMetrics m;
    m.crashes = crashCount;
    m.maxLatencyMs = maxLatency > 0.0 ? maxLatency : baseLatencyMs;
    m.packetLossPct = lossPct;
    m.peerDrops = peerDrops;
    m.finalityLag = finalityLag;
    m.rto = timeToRecover;
    return m;
}

bool ChaosEngine::isClusterHealthy() const {
    int aliveCount = 0; for (bool a : alive) if (a) aliveCount++;
    bool liveness = aliveCount >= 2;
    bool boundedLag = finalityLag < std::chrono::seconds(10);
    bool recovering = timeToRecover < std::chrono::seconds(15);
    return liveness && boundedLag && recovering && !diskError;
}

