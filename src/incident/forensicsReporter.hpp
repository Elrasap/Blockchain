#include "incident/forensics_reporter.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <chrono>
#include <ctime>

ForensicsReporter::ForensicsReporter() {}

std::string ForensicsReporter::hashString(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.size(), hash);
    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

void ForensicsReporter::writeReport(const Incident& inc, const std::string& dir) {
    auto ts = std::chrono::system_clock::to_time_t(inc.timestamp);
    std::ostringstream fname;
    fname << dir << "/incident_" << ts << ".json";

    std::ostringstream payload;
    payload << inc.type << "|" << inc.severity << "|" << inc.message;
    std::string digest = hashString(payload.str());

    std::ofstream out(fname.str());
    out << "{\n";
    out << "  \"timestamp\": \"" << std::ctime(&ts) << "\",\n";
    out << "  \"type\": \"" << inc.type << "\",\n";
    out << "  \"severity\": \"" << inc.severity << "\",\n";
    out << "  \"message\": \"" << inc.message << "\",\n";
    out << "  \"state_hash\": \"" << digest << "\"\n";
    out << "}\n";
}

