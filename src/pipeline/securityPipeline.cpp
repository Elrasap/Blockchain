#include "pipeline/securityPipeline.hpp"
#include <fstream>
#include <chrono>
#include <ctime>
#include <iostream>

SecurityPipeline::SecurityPipeline(const std::string& releaseDir)
    : dir(releaseDir) {}

void SecurityPipeline::logResult(const ScanResult& r, const PolicyEnforcer& e) {
    std::ofstream log("security_gate.log", std::ios::app);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    log << "\n=== Security Audit " << std::ctime(&now) << " ===\n";
    log << "Checksum: "   << (r.checksumValid ? "OK" : "FAIL") << "\n";
    log << "Signature: "  << (r.signatureValid ? "OK" : "FAIL") << "\n";
    log << "SBOM: "       << (r.sbomValid ? "OK" : "FAIL") << "\n";
    log << "Attestation: "<< (r.attestationValid ? "OK" : "FAIL") << "\n";
    log << "Vulns: "      << r.vulnerabilities.size() << "\n";
    log << "Policy: "     << (e.allPassed() ? "PASSED" : "FAILED") << "\n";
}

bool SecurityPipeline::run() {
    SecurityScanner scanner(dir);
    ScanResult res = scanner.run();

    bool integrity = res.checksumValid && res.signatureValid &&
                     res.sbomValid && res.attestationValid;

    PolicyEnforcer enforcer;
    enforcer.evaluate(res.vulnerabilities, integrity);

    AuditReport::writeJson("last_audit.json", enforcer.getResults());
    logResult(res, enforcer);

    if (!enforcer.allPassed()) {
        std::cerr << "[SecurityPipeline] Audit failed – deployment blocked\n";
        return false;
    }
    std::cout << "[SecurityPipeline] Audit passed – deployment allowed\n";
    return true;
}

