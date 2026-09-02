#include "core/transaction.hpp"
#include "core/crypto.hpp"
#include <algorithm>
#include <stdexcept>

namespace {

constexpr std::array<uint8_t, 4> TX_MAGIC{'D', 'N', 'T', 'X'};
constexpr uint16_t TX_VERSION = 1;
constexpr uint32_t CHAIN_ID = 1;

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

bool readU16(const std::vector<uint8_t>& data, std::size_t& offset, uint16_t& value)
{
    if (data.size() - offset < 2) return false;
    value = static_cast<uint16_t>(data[offset]) |
            static_cast<uint16_t>(data[offset + 1]) << 8;
    offset += 2;
    return true;
}

bool readU32(const std::vector<uint8_t>& data, std::size_t& offset, uint32_t& value)
{
    if (data.size() - offset < 4) return false;
    value = 0;
    for (int i = 0; i < 4; ++i)
        value |= static_cast<uint32_t>(data[offset + i]) << (8 * i);
    offset += 4;
    return true;
}

bool readU64(const std::vector<uint8_t>& data, std::size_t& offset, uint64_t& value)
{
    if (data.size() - offset < 8) return false;
    value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<uint64_t>(data[offset + i]) << (8 * i);
    offset += 8;
    return true;
}

} // namespace

std::vector<uint8_t> Transaction::serializeForSigning() const
{
    if (senderPubkey.size() != PUBLIC_KEY_SIZE)
        throw std::runtime_error("transaction: invalid public key size");
    if (payload.size() > MAX_PAYLOAD_SIZE)
        throw std::runtime_error("transaction: payload too large");

    std::vector<uint8_t> out;
    out.reserve(4 + 2 + 4 + PUBLIC_KEY_SIZE + 8 + 8 + 4 + payload.size());
    out.insert(out.end(), TX_MAGIC.begin(), TX_MAGIC.end());
    writeU16(out, TX_VERSION);
    writeU32(out, CHAIN_ID);
    out.insert(out.end(), senderPubkey.begin(), senderPubkey.end());
    writeU64(out, nonce);
    writeU64(out, fee);
    writeU32(out, static_cast<uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<uint8_t> Transaction::serialize() const
{
    if (!signature.empty() && signature.size() != SIGNATURE_SIZE)
        throw std::runtime_error("transaction: invalid signature size");

    auto out = serializeForSigning();
    writeU16(out, static_cast<uint16_t>(signature.size()));
    out.insert(out.end(), signature.begin(), signature.end());
    return out;
}

std::array<uint8_t, 32> Transaction::hash() const
{
    return crypto::sha256(serialize());
}

void Transaction::sign(const std::vector<uint8_t>& priv)
{
    signature = crypto::sign(serializeForSigning(), priv);
}

bool Transaction::verifySignature() const
{
    if (senderPubkey.size() != PUBLIC_KEY_SIZE ||
        signature.size() != SIGNATURE_SIZE)
        return false;

    try {
        return crypto::verify(serializeForSigning(), signature, senderPubkey);
    } catch (...) {
        return false;
    }
}

bool Transaction::deserialize(const std::vector<uint8_t>& data,
                              Transaction& out,
                              std::string& error)
{
    const std::size_t minimumSize = 4 + 2 + 4 + PUBLIC_KEY_SIZE + 8 + 8 + 4 + 2;
    if (data.size() < minimumSize) {
        error = "transaction: truncated header";
        return false;
    }

    std::size_t offset = 0;
    if (!std::equal(TX_MAGIC.begin(), TX_MAGIC.end(), data.begin())) {
        error = "transaction: invalid magic";
        return false;
    }
    offset += TX_MAGIC.size();

    uint16_t version = 0;
    uint32_t chainId = 0;
    if (!readU16(data, offset, version) || version != TX_VERSION) {
        error = "transaction: unsupported version";
        return false;
    }
    if (!readU32(data, offset, chainId) || chainId != CHAIN_ID) {
        error = "transaction: wrong chain id";
        return false;
    }

    Transaction decoded;
    decoded.senderPubkey.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                                data.begin() + static_cast<std::ptrdiff_t>(offset + PUBLIC_KEY_SIZE));
    offset += PUBLIC_KEY_SIZE;

    uint32_t payloadSize = 0;
    if (!readU64(data, offset, decoded.nonce) ||
        !readU64(data, offset, decoded.fee) ||
        !readU32(data, offset, payloadSize)) {
        error = "transaction: truncated body";
        return false;
    }
    if (payloadSize > MAX_PAYLOAD_SIZE || payloadSize > data.size() - offset) {
        error = "transaction: invalid payload length";
        return false;
    }

    decoded.payload.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                           data.begin() + static_cast<std::ptrdiff_t>(offset + payloadSize));
    offset += payloadSize;

    uint16_t signatureSize = 0;
    if (!readU16(data, offset, signatureSize) ||
        (signatureSize != 0 && signatureSize != SIGNATURE_SIZE) ||
        signatureSize > data.size() - offset) {
        error = "transaction: invalid signature length";
        return false;
    }
    decoded.signature.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                             data.begin() + static_cast<std::ptrdiff_t>(offset + signatureSize));
    offset += signatureSize;

    if (offset != data.size()) {
        error = "transaction: trailing bytes";
        return false;
    }

    out = std::move(decoded);
    return true;
}

void Transaction::deserialize(const std::vector<uint8_t>& data)
{
    std::string error;
    Transaction decoded;
    if (!deserialize(data, decoded, error))
        throw std::runtime_error(error);
    *this = std::move(decoded);
}
