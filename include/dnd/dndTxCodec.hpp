// include/dnd/dndTxCodec.hpp
#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "dnd/dndTx.hpp"

namespace dnd {

/**
 * Binäre Kodierung eines DndEventTx.
 *
 * Format (Little Endian):
 *  [u8]   version (z.B. 1)
 *  [u16]  len(encounterId) + bytes
 *  [u16]  len(actorId)     + bytes
 *  [u8]   actorType
 *  [u16]  len(targetId)    + bytes
 *  [u8]   targetType
 *  [u8]   hit (0/1)
 *  [i32]  damage
 *  [u64]  timestamp
 *  [u32]  round
 *  [u32]  turnIndex
 *  [u16]  len(sourceId)    + bytes
 *
 * senderPubKey + signature werden NICHT mitkodiert; die kommen von der umgebenden Tx.
 */

std::vector<std::uint8_t> encodeDndTx(const DndEventTx& evt);

// einfache Variante: wirft bei Fehlern eine std::runtime_error
DndEventTx decodeDndTx(const std::vector<std::uint8_t>& buf);

// sichere Variante mit Fehlerstring (falls du sie mal brauchst)
bool decodeDndTx(const std::vector<std::uint8_t>& buf,
                 DndEventTx& out,
                 std::string& err);

inline std::vector<uint8_t> encodeDndTx(const DndEventTx& evt)
{
    std::vector<uint8_t> out;

    out.push_back('D');
    out.push_back('N');
    out.push_back('D');
    out.push_back('1');

    auto appendStr = [&](const std::string& s) {
        uint32_t len = s.size();
        out.insert(out.end(), (uint8_t*)&len, (uint8_t*)&len + 4);
        out.insert(out.end(), s.begin(), s.end());
    };

    auto appendInt64 = [&](uint64_t v) {
        out.insert(out.end(), (uint8_t*)&v, (uint8_t*)&v + 8);
    };

    appendStr(evt.encounterId);
    appendStr(evt.actorId);
    appendStr(evt.targetId);

    appendInt64(evt.actorType);
    appendInt64(evt.targetType);
    appendInt64(evt.roll);
    appendInt64(evt.damage);
    appendInt64(evt.hit ? 1 : 0);
    appendStr(evt.note);
    appendInt64(evt.timestamp);

    // pubkey
    uint32_t pks = evt.senderPubKey.size();
    out.insert(out.end(), (uint8_t*)&pks, (uint8_t*)&pks + 4);
    out.insert(out.end(), evt.senderPubKey.begin(), evt.senderPubKey.end());

    // signature
    uint32_t sigs = evt.signature.size();
    out.insert(out.end(), (uint8_t*)&sigs, (uint8_t*)&sigs + 4);
    out.insert(out.end(), evt.signature.begin(), evt.signature.end());

    return out;
}

// ---- DECODE ----
inline DndEventTx decodeDndTx(const std::vector<uint8_t>& buf)
{
    DndEventTx evt;

    if (buf.size() < 4) return evt;
    if (!(buf[0]=='D' && buf[1]=='N' && buf[2]=='D' && buf[3]=='1'))
        return evt;

    size_t p = 4;

    auto readStr = [&](std::string& s) {
        uint32_t len;
        memcpy(&len, &buf[p], 4);
        p += 4;
        s.assign((const char*)&buf[p], len);
        p += len;
    };

    auto readInt64 = [&](uint64_t& v) {
        memcpy(&v, &buf[p], 8);
        p += 8;
    };

    readStr(evt.encounterId);
    readStr(evt.actorId);
    readStr(evt.targetId);

    uint64_t tmp;

    readInt64(tmp); evt.actorType  = (int)tmp;
    readInt64(tmp); evt.targetType = (int)tmp;
    readInt64(tmp); evt.roll       = (int)tmp;
    readInt64(tmp); evt.damage     = (int)tmp;

    readInt64(tmp);
    evt.hit = (tmp != 0);

    readStr(evt.note);

    readInt64(tmp);
    evt.timestamp = tmp;

    // pubkey
    uint32_t pks;
    memcpy(&pks, &buf[p], 4);
    p += 4;
    evt.senderPubKey.assign(buf.begin() + p, buf.begin() + p + pks);
    p += pks;

    // signature
    uint32_t sigs;
    memcpy(&sigs, &buf[p], 4);
    p += 4;
    evt.signature.assign(buf.begin() + p, buf.begin() + p + sigs);
    p += sigs;

    return evt;
}

} // namespace dnd

