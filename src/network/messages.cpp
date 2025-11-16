#include "network/messages.hpp"
#include <cstring>
#include "core/blockEncoding.hpp"
#include "core/blockEncoding.hpp"


static void appendBytes(std::vector<uint8_t>& out, const void* src, size_t len) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(src);
    out.insert(out.end(), p, p + len);
}

static void readBytes(const std::vector<uint8_t>& in, size_t& pos, void* dst, size_t len) {
    std::memcpy(dst, in.data() + pos, len);
    pos += len;
}

#include "network/messages.hpp"
#include <cstring>

std::vector<uint8_t> encodeGetHeader(uint64_t height) {
    std::vector<uint8_t> buf(8);
    for (int i = 0; i < 8; ++i)
        buf[i] = static_cast<uint8_t>((height >> (8 * i)) & 0xFF);
    return buf;
}

uint64_t decodeGetHeader(const std::vector<uint8_t>& bytes) {
    uint64_t h = 0;
    size_t n = bytes.size() < 8 ? bytes.size() : 8;
    for (size_t i = 0; i < n; ++i)
        h |= (static_cast<uint64_t>(bytes[i]) << (8 * i));
    return h;
}

std::vector<uint8_t> encodeGetProofTx(const std::array<uint8_t, 32>& txHash) {
    std::vector<uint8_t> buf(txHash.begin(), txHash.end());
    return buf;
}

std::array<uint8_t, 32> decodeGetProofTx(const std::vector<uint8_t>& bytes) {
    std::array<uint8_t, 32> hash{};
    size_t n = bytes.size() < 32 ? bytes.size() : 32;
    std::memcpy(hash.data(), bytes.data(), n);
    return hash;
}


std::vector<uint8_t> encodeMerkleProof(const MerkleProof& p) {
    std::vector<uint8_t> out;
    appendBytes(out, p.leaf.data(), 32);
    uint64_t n = p.path.size();
    appendBytes(out, &n, sizeof(uint64_t));
    for (auto& h : p.path) appendBytes(out, h.data(), 32);
    for (bool b : p.left) {
        uint8_t bit = b ? 1 : 0;
        appendBytes(out, &bit, 1);
    }
    appendBytes(out, p.root.data(), 32);
    return out;
}

MerkleProof decodeMerkleProof(const std::vector<uint8_t>& bytes) {
    MerkleProof p;
    size_t pos = 0;
    readBytes(bytes, pos, p.leaf.data(), 32);
    uint64_t n = 0;
    readBytes(bytes, pos, &n, sizeof(uint64_t));
    p.path.resize(n);
    for (uint64_t i = 0; i < n; ++i)
        readBytes(bytes, pos, p.path[i].data(), 32);
    p.left.resize(n);
    for (uint64_t i = 0; i < n; ++i) {
        uint8_t bit = 0;
        readBytes(bytes, pos, &bit, 1);
        p.left[i] = (bit != 0);
    }
    readBytes(bytes, pos, p.root.data(), 32);
    return p;
}

std::vector<uint8_t> encodeMessage(const Message& msg) {
    std::vector<uint8_t> out;
    uint8_t t = static_cast<uint8_t>(msg.type);
    appendBytes(out, &t, 1);
    uint64_t len = msg.payload.size();
    appendBytes(out, &len, sizeof(uint64_t));
    appendBytes(out, msg.payload.data(), len);
    return out;
}

Message decodeMessage(const std::vector<uint8_t>& bytes) {
    Message msg;
    size_t pos = 0;
    uint8_t t = 0;
    readBytes(bytes, pos, &t, 1);
    msg.type = static_cast<MessageType>(t);
    uint64_t len = 0;
    readBytes(bytes, pos, &len, sizeof(uint64_t));
    msg.payload.resize(len);
    readBytes(bytes, pos, msg.payload.data(), len);
    return msg;
}

#include "network/messages.hpp"
#include <cstring>

