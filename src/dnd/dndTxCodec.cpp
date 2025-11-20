#include "dnd/dndTxCodec.hpp"

#include <stdexcept>
#include <string>
#include <cstdint>

namespace dnd {

namespace {

// ---------- kleine Helfer: Varuint + LE-Encode ----------

void writeVarUint32(std::vector<uint8_t>& out, uint32_t v) {
    // simple LEB128-Varint
    while (v >= 0x80) {
        out.push_back(static_cast<uint8_t>((v & 0x7F) | 0x80));
        v >>= 7;
    }
    out.push_back(static_cast<uint8_t>(v));
}

uint32_t readVarUint32(const std::vector<uint8_t>& buf, size_t& off) {
    uint32_t result = 0;
    int shift = 0;

    while (true) {
        if (off >= buf.size())
            throw std::runtime_error("decodeDndTx: varuint32 out of range");

        uint8_t byte = buf[off++];
        result |= static_cast<uint32_t>(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0)
            break;

        shift += 7;
        if (shift > 28) {
            throw std::runtime_error("decodeDndTx: varuint32 too large");
        }
    }
    return result;
}

void writeInt32LE(std::vector<uint8_t>& out, int32_t v) {
    uint32_t u = static_cast<uint32_t>(v);
    out.push_back(static_cast<uint8_t>(u & 0xFF));
    out.push_back(static_cast<uint8_t>((u >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((u >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((u >> 24) & 0xFF));
}

int32_t readInt32LE(const std::vector<uint8_t>& buf, size_t& off) {
    if (off + 4 > buf.size())
        throw std::runtime_error("decodeDndTx: int32 out of range");

    uint32_t u = 0;
    u |= static_cast<uint32_t>(buf[off++]);
    u |= static_cast<uint32_t>(buf[off++]) << 8;
    u |= static_cast<uint32_t>(buf[off++]) << 16;
    u |= static_cast<uint32_t>(buf[off++]) << 24;
    return static_cast<int32_t>(u);
}

void writeUint64LE(std::vector<uint8_t>& out, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>(v & 0xFF));
        v >>= 8;
    }
}

uint64_t readUint64LE(const std::vector<uint8_t>& buf, size_t& off) {
    if (off + 8 > buf.size())
        throw std::runtime_error("decodeDndTx: uint64 out of range");

    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<uint64_t>(buf[off++]) << (8 * i);
    }
    return v;
}

void writeString(std::vector<uint8_t>& out, const std::string& s) {
    writeVarUint32(out, static_cast<uint32_t>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

std::string readString(const std::vector<uint8_t>& buf, size_t& off) {
    uint32_t len = readVarUint32(buf, off);
    if (off + len > buf.size())
        throw std::runtime_error("decodeDndTx: string length out of range");

    std::string s;
    s.assign(reinterpret_cast<const char*>(&buf[off]), len);
    off += len;
    return s;
}

} // namespace

// -------------------------------------------------------------
// Encode DnD TX → kompaktes Binary
// -------------------------------------------------------------
std::vector<uint8_t> encodeDndTx(const DndEventTx& tx)
{
    std::vector<uint8_t> out;
    out.reserve(64);

    // Magic
    out.push_back(0xD1);

    // encounterId
    writeString(out, tx.encounterId);

    // actorType + actorId
    out.push_back(static_cast<uint8_t>(tx.actorType));
    writeString(out, tx.actorId);

    // targetType + targetId
    out.push_back(static_cast<uint8_t>(tx.targetType));
    writeString(out, tx.targetId);

    // roll, damage, hit
    writeInt32LE(out, tx.roll);
    writeInt32LE(out, tx.damage);
    out.push_back(static_cast<uint8_t>(tx.hit ? 1 : 0));

    // note
    writeString(out, tx.note);

    // timestamp
    writeUint64LE(out, tx.timestamp);

    return out;
}

// -------------------------------------------------------------
// Decode Binary → DndEventTx (ohne Signatur / senderPubKey)
// -------------------------------------------------------------
DndEventTx decodeDndTx(const std::vector<uint8_t>& buf)
{
    if (buf.empty())
        throw std::runtime_error("decodeDndTx: empty buffer");

    size_t off = 0;

    uint8_t magic = buf[off++];
    if (magic != 0xD1) {
        throw std::runtime_error("decodeDndTx: invalid magic byte (not DnD payload)");
    }

    DndEventTx tx;

    tx.encounterId = readString(buf, off);

    tx.actorType = 0;
    if (off >= buf.size())
        throw std::runtime_error("decodeDndTx: missing actorType");
    tx.actorType = buf[off++];

    tx.actorId = readString(buf, off);

    tx.targetType = 0;
    if (off >= buf.size())
        throw std::runtime_error("decodeDndTx: missing targetType");
    tx.targetType = buf[off++];

    tx.targetId = readString(buf, off);

    tx.roll   = readInt32LE(buf, off);
    tx.damage = readInt32LE(buf, off);

    if (off >= buf.size())
        throw std::runtime_error("decodeDndTx: missing hit flag");
    tx.hit = (buf[off++] != 0);

    tx.note      = readString(buf, off);
    tx.timestamp = readUint64LE(buf, off);

    // Diese Felder werden extern aus der Transaction gesetzt:
    tx.senderPubKey.clear();
    tx.signature.clear();

    return tx;
}

} // namespace dnd

