#include "core/blockJson.hpp"
#include <string>
#include <vector>
#include <array>
#include <stdexcept>

using nlohmann::json;

// -------------------------------------------------------
// Hilfsfunktionen: Bytes <-> Hex
// -------------------------------------------------------
static std::string bytesToHex(const std::vector<uint8_t>& data) {
    static const char* HEX = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (uint8_t b : data) {
        out.push_back(HEX[b >> 4]);
        out.push_back(HEX[b & 0x0F]);
    }
    return out;
}

static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        throw std::runtime_error("hexToBytes: invalid hex length");
    }
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto h1 = hex[i];
        auto h2 = hex[i + 1];

        auto nib = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
            return -1;
        };

        int n1 = nib(h1);
        int n2 = nib(h2);
        if (n1 < 0 || n2 < 0) {
            throw std::runtime_error("hexToBytes: invalid hex char");
        }
        out.push_back(static_cast<uint8_t>((n1 << 4) | n2));
    }
    return out;
}

static std::string array32ToHex(const std::array<uint8_t, 32>& a) {
    std::vector<uint8_t> v(a.begin(), a.end());
    return bytesToHex(v);
}

static std::array<uint8_t, 32> hexToArray32(const std::string& hex) {
    auto v = hexToBytes(hex);
    if (v.size() != 32) {
        throw std::runtime_error("hexToArray32: expected 32 bytes");
    }
    std::array<uint8_t, 32> a{};
    std::copy(v.begin(), v.end(), a.begin());
    return a;
}

// -------------------------------------------------------
// BlockHeader <-> JSON
// -------------------------------------------------------
void to_json(json& j, const BlockHeader& h) {
    j = json{
        {"height",           h.height},
        {"timestamp",        h.timestamp},
        {"prevHashHex",      array32ToHex(h.prevHash)},
        {"merkleRootHex",    array32ToHex(h.merkleRoot)},
        {"validatorPubKey",  bytesToHex(h.validatorPubKey)},
        {"signature",        bytesToHex(h.signature)}
    };
}

void from_json(const json& j, BlockHeader& h) {
    h.height    = j.at("height").get<uint64_t>();
    h.timestamp = j.at("timestamp").get<uint64_t>();

    auto prevHex   = j.at("prevHashHex").get<std::string>();
    auto merkleHex = j.at("merkleRootHex").get<std::string>();

    h.prevHash   = hexToArray32(prevHex);
    h.merkleRoot = hexToArray32(merkleHex);

    auto valHex = j.value("validatorPubKey", std::string{});
    auto sigHex = j.value("signature", std::string{});

    h.validatorPubKey = valHex.empty() ? std::vector<uint8_t>{}
                                       : hexToBytes(valHex);
    h.signature       = sigHex.empty() ? std::vector<uint8_t>{}
                                       : hexToBytes(sigHex);
}

// -------------------------------------------------------
// Transaction <-> JSON (nur für Persistenz)
// -------------------------------------------------------
static void to_json(json& j, const Transaction& tx) {
    j = json{
        {"senderPubkey", bytesToHex(tx.senderPubkey)},
        {"payload",      bytesToHex(tx.payload)},
        {"signature",    bytesToHex(tx.signature)},
        {"nonce",        tx.nonce},
        {"fee",          tx.fee}
    };
}

static void from_json(const json& j, Transaction& tx) {
    auto senderHex = j.value("senderPubkey", std::string{});
    auto payloadHex = j.value("payload", std::string{});
    auto sigHex = j.value("signature", std::string{});

    tx.senderPubkey = senderHex.empty() ? std::vector<uint8_t>{}
                                        : hexToBytes(senderHex);
    tx.payload      = payloadHex.empty() ? std::vector<uint8_t>{}
                                         : hexToBytes(payloadHex);
    tx.signature    = sigHex.empty() ? std::vector<uint8_t>{}
                                     : hexToBytes(sigHex);

    tx.nonce = j.value("nonce", uint64_t{0});
    tx.fee   = j.value("fee",   uint64_t{0});
}

// -------------------------------------------------------
// Block <-> JSON
// -------------------------------------------------------
void to_json(json& j, const Block& b) {
    j = json{
        {"header",       b.header},
        {"transactions", json::array()}
    };

    for (const auto& tx : b.transactions) {
        json jTx;
        to_json(jTx, tx);
        j["transactions"].push_back(jTx);
    }
}

void from_json(const json& j, Block& b) {
    b.header = j.at("header").get<BlockHeader>();

    b.transactions.clear();
    if (j.contains("transactions")) {
        for (const auto& jTx : j.at("transactions")) {
            Transaction tx;
            from_json(jTx, tx);
            b.transactions.push_back(std::move(tx));
        }
    }
}

