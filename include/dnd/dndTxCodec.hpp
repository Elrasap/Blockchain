#pragma once

#include <vector>
#include <cstdint>

#include "dnd/dndTx.hpp"

namespace dnd {

// Encode DnDEventTx into a compact binary payload:
// [0]         = 0xD1  (magic)
// varuint32   = len(encounterId), then bytes
// uint8       = actorType
// varuint32   = len(actorId), then bytes
// uint8       = targetType
// varuint32   = len(targetId), then bytes
// int32_le    = roll
// int32_le    = damage
// uint8       = hit (0/1)
// varuint32   = len(note), then bytes
// uint64_le   = timestamp
//
// WICHTIG: senderPubKey und signature werden NICHT serialisiert.
// Die Signatur liegt NUR auf der Transaction.
std::vector<uint8_t> encodeDndTx(const DndEventTx& tx);

// Reverse of encodeDndTx. Throws std::runtime_error on invalid data.
// senderPubKey & signature bleiben leer – werden extern aus der Transaction gesetzt.
DndEventTx decodeDndTx(const std::vector<uint8_t>& buf);

} // namespace dnd

