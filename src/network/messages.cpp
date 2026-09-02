#include "network/messages.hpp"
#include "core/blockEncoding.hpp"

#include <algorithm>
#include <stdexcept>

namespace {

constexpr std::array<uint8_t, 4> MESSAGE_MAGIC{'D', 'N', 'D', 'P'};
constexpr uint16_t PROTOCOL_VERSION = 1;
constexpr std::size_t MAX_PROOF_PATH = 64;

void writeU16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
}

void writeU32(std::vector<uint8_t>& out, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
        out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}

void writeU64(std::vector<uint8_t>& out, uint64_t value)
{
    for (int i = 0; i < 8; ++i)
        out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}

bool readU32(const std::vector<uint8_t>& in, std::size_t& offset, uint32_t& value)
{
    if (in.size() - offset < 4) return false;
    value = 0;
    for (int i = 0; i < 4; ++i)
        value |= static_cast<uint32_t>(in[offset + i]) << (8 * i);
    offset += 4;
    return true;
}

bool readU64(const std::vector<uint8_t>& in, std::size_t& offset, uint64_t& value)
{
    if (in.size() - offset < 8) return false;
    value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<uint64_t>(in[offset + i]) << (8 * i);
    offset += 8;
    return true;
}

bool validMessageType(uint8_t type)
{
    return type >= static_cast<uint8_t>(MessageType::TX_BROADCAST) &&
           type <= static_cast<uint8_t>(MessageType::GETPROOF_TX);
}

} // namespace

std::vector<uint8_t> encodeMessage(const Message& msg)
{
    const auto type = static_cast<uint8_t>(msg.type);
    if (!validMessageType(type))
        throw std::runtime_error("message: invalid type");
    if (msg.payload.size() > MAX_MESSAGE_PAYLOAD)
        throw std::runtime_error("message: payload too large");

    std::vector<uint8_t> out;
    out.reserve(MESSAGE_HEADER_SIZE + msg.payload.size());
    out.insert(out.end(), MESSAGE_MAGIC.begin(), MESSAGE_MAGIC.end());
    writeU16(out, PROTOCOL_VERSION);
    out.push_back(type);
    writeU32(out, static_cast<uint32_t>(msg.payload.size()));
    out.insert(out.end(), msg.payload.begin(), msg.payload.end());
    return out;
}

FrameDecodeResult tryDecodeMessageFrame(const std::vector<uint8_t>& buffer,
                                        Message& out,
                                        std::size_t& consumed,
                                        std::string& error)
{
    consumed = 0;
    error.clear();
    if (buffer.size() < MESSAGE_HEADER_SIZE)
        return FrameDecodeResult::NeedMoreData;

    if (!std::equal(MESSAGE_MAGIC.begin(), MESSAGE_MAGIC.end(), buffer.begin())) {
        error = "message: invalid magic";
        return FrameDecodeResult::Invalid;
    }

    const uint16_t version = static_cast<uint16_t>(buffer[4]) |
                             static_cast<uint16_t>(buffer[5]) << 8;
    if (version != PROTOCOL_VERSION) {
        error = "message: unsupported protocol version";
        return FrameDecodeResult::Invalid;
    }
    if (!validMessageType(buffer[6])) {
        error = "message: invalid type";
        return FrameDecodeResult::Invalid;
    }

    std::size_t offset = 7;
    uint32_t payloadSize = 0;
    if (!readU32(buffer, offset, payloadSize)) {
        error = "message: truncated payload length";
        return FrameDecodeResult::Invalid;
    }
    if (payloadSize > MAX_MESSAGE_PAYLOAD) {
        error = "message: payload length exceeds limit";
        return FrameDecodeResult::Invalid;
    }

    const std::size_t frameSize = MESSAGE_HEADER_SIZE + payloadSize;
    if (buffer.size() < frameSize)
        return FrameDecodeResult::NeedMoreData;

    Message decoded;
    decoded.type = static_cast<MessageType>(buffer[6]);
    decoded.payload.assign(buffer.begin() + static_cast<std::ptrdiff_t>(MESSAGE_HEADER_SIZE),
                           buffer.begin() + static_cast<std::ptrdiff_t>(frameSize));
    out = std::move(decoded);
    consumed = frameSize;
    return FrameDecodeResult::Complete;
}

bool decodeMessage(const std::vector<uint8_t>& bytes,
                   Message& out,
                   std::string& error)
{
    std::size_t consumed = 0;
    const auto result = tryDecodeMessageFrame(bytes, out, consumed, error);
    if (result == FrameDecodeResult::NeedMoreData) {
        error = "message: truncated frame";
        return false;
    }
    if (result == FrameDecodeResult::Invalid)
        return false;
    if (consumed != bytes.size()) {
        error = "message: trailing bytes";
        return false;
    }
    return true;
}

Message decodeMessage(const std::vector<uint8_t>& bytes)
{
    Message out;
    std::string error;
    if (!decodeMessage(bytes, out, error))
        throw std::runtime_error(error);
    return out;
}

std::vector<uint8_t> encodeGetHeader(uint64_t height)
{
    std::vector<uint8_t> out;
    writeU64(out, height);
    return out;
}

uint64_t decodeGetHeader(const std::vector<uint8_t>& bytes)
{
    if (bytes.size() != 8)
        throw std::runtime_error("get-header: expected 8 bytes");
    std::size_t offset = 0;
    uint64_t height = 0;
    readU64(bytes, offset, height);
    return height;
}

std::vector<uint8_t> encodeGetProofTx(const std::array<uint8_t, 32>& txHash)
{
    return {txHash.begin(), txHash.end()};
}

std::array<uint8_t, 32> decodeGetProofTx(const std::vector<uint8_t>& bytes)
{
    if (bytes.size() != 32)
        throw std::runtime_error("get-proof: expected 32 bytes");
    std::array<uint8_t, 32> hash{};
    std::copy(bytes.begin(), bytes.end(), hash.begin());
    return hash;
}

std::vector<uint8_t> encodeMerkleProof(const MerkleProof& proof)
{
    if (proof.path.size() != proof.left.size() ||
        proof.path.size() > MAX_PROOF_PATH)
        throw std::runtime_error("merkle proof: invalid path");

    std::vector<uint8_t> out;
    out.insert(out.end(), proof.leaf.begin(), proof.leaf.end());
    writeU32(out, static_cast<uint32_t>(proof.path.size()));
    for (const auto& hash : proof.path)
        out.insert(out.end(), hash.begin(), hash.end());
    for (bool left : proof.left)
        out.push_back(left ? 1 : 0);
    out.insert(out.end(), proof.root.begin(), proof.root.end());
    return out;
}

MerkleProof decodeMerkleProof(const std::vector<uint8_t>& bytes)
{
    constexpr std::size_t baseSize = 32 + 4 + 32;
    if (bytes.size() < baseSize)
        throw std::runtime_error("merkle proof: truncated");

    MerkleProof proof;
    std::size_t offset = 0;
    std::copy_n(bytes.begin(), 32, proof.leaf.begin());
    offset += 32;

    uint32_t pathSize = 0;
    readU32(bytes, offset, pathSize);
    if (pathSize > MAX_PROOF_PATH ||
        pathSize > (bytes.size() - offset - 32) / 33 ||
        bytes.size() != baseSize + static_cast<std::size_t>(pathSize) * 33)
        throw std::runtime_error("merkle proof: invalid path length");

    proof.path.resize(pathSize);
    for (auto& hash : proof.path) {
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), 32,
                    hash.begin());
        offset += 32;
    }

    proof.left.resize(pathSize);
    for (std::size_t i = 0; i < pathSize; ++i) {
        if (bytes[offset] > 1)
            throw std::runtime_error("merkle proof: invalid direction bit");
        proof.left[i] = bytes[offset++] != 0;
    }

    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), 32,
                proof.root.begin());
    return proof;
}
