#include "core/block.hpp"
#include "core/crypto.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {

constexpr std::array<uint8_t, 4> HEADER_MAGIC{'D', 'N', 'B', 'H'};
constexpr std::array<uint8_t, 4> BLOCK_MAGIC{'D', 'N', 'B', 'L'};
constexpr uint16_t HEADER_VERSION = 1;
constexpr uint16_t BLOCK_VERSION = 1;

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

bool decodeHeaderBytes(const std::vector<uint8_t>& data,
                       BlockHeader& out,
                       std::string& error)
{
    constexpr std::size_t fixedSize = 4 + 2 + 8 + 8 + 32 + 32 + 2 + 2;
    if (data.size() < fixedSize) {
        error = "block header: truncated";
        return false;
    }

    std::size_t offset = 0;
    if (!std::equal(HEADER_MAGIC.begin(), HEADER_MAGIC.end(), data.begin())) {
        error = "block header: invalid magic";
        return false;
    }
    offset += HEADER_MAGIC.size();

    uint16_t version = 0;
    if (!readU16(data, offset, version) || version != HEADER_VERSION) {
        error = "block header: unsupported version";
        return false;
    }

    BlockHeader decoded;
    if (!readU64(data, offset, decoded.height) ||
        !readU64(data, offset, decoded.timestamp)) {
        error = "block header: truncated fields";
        return false;
    }

    std::copy_n(data.begin() + static_cast<std::ptrdiff_t>(offset), 32,
                decoded.prevHash.begin());
    offset += 32;
    std::copy_n(data.begin() + static_cast<std::ptrdiff_t>(offset), 32,
                decoded.merkleRoot.begin());
    offset += 32;

    uint16_t publicKeySize = 0;
    if (!readU16(data, offset, publicKeySize) ||
        publicKeySize > data.size() - offset) {
        error = "block header: invalid public key length";
        return false;
    }
    decoded.validatorPubKey.assign(
        data.begin() + static_cast<std::ptrdiff_t>(offset),
        data.begin() + static_cast<std::ptrdiff_t>(offset + publicKeySize));
    offset += publicKeySize;

    uint16_t signatureSize = 0;
    if (!readU16(data, offset, signatureSize) ||
        signatureSize > data.size() - offset) {
        error = "block header: invalid signature length";
        return false;
    }
    decoded.signature.assign(
        data.begin() + static_cast<std::ptrdiff_t>(offset),
        data.begin() + static_cast<std::ptrdiff_t>(offset + signatureSize));
    offset += signatureSize;

    if (offset != data.size()) {
        error = "block header: trailing bytes";
        return false;
    }

    out = std::move(decoded);
    return true;
}

} // namespace

std::vector<uint8_t> BlockHeader::toSigningBytes() const
{
    if (validatorPubKey.size() > std::numeric_limits<uint16_t>::max())
        throw std::runtime_error("block header: public key too large");

    std::vector<uint8_t> out;
    out.reserve(4 + 2 + 8 + 8 + 32 + 32 + 2 + validatorPubKey.size());
    out.insert(out.end(), HEADER_MAGIC.begin(), HEADER_MAGIC.end());
    writeU16(out, HEADER_VERSION);
    writeU64(out, height);
    writeU64(out, timestamp);
    out.insert(out.end(), prevHash.begin(), prevHash.end());
    out.insert(out.end(), merkleRoot.begin(), merkleRoot.end());
    writeU16(out, static_cast<uint16_t>(validatorPubKey.size()));
    out.insert(out.end(), validatorPubKey.begin(), validatorPubKey.end());
    return out;
}

std::vector<uint8_t> BlockHeader::toBytes() const
{
    if (signature.size() > std::numeric_limits<uint16_t>::max())
        throw std::runtime_error("block header: signature too large");

    auto out = toSigningBytes();
    writeU16(out, static_cast<uint16_t>(signature.size()));
    out.insert(out.end(), signature.begin(), signature.end());
    return out;
}

std::array<uint8_t, 32> BlockHeader::hash() const
{
    return crypto::sha256(toBytes());
}

bool BlockHeader::deserialize(const std::vector<uint8_t>& data,
                              BlockHeader& out,
                              std::string& error)
{
    return decodeHeaderBytes(data, out, error);
}

std::array<uint8_t, 32> Block::calculateMerkleRoot() const
{
    if (transactions.empty())
        return crypto::sha256({});

    std::vector<std::array<uint8_t, 32>> layer;
    layer.reserve(transactions.size());
    for (const auto& tx : transactions)
        layer.push_back(tx.hash());

    while (layer.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next;
        next.reserve((layer.size() + 1) / 2);
        for (std::size_t i = 0; i < layer.size(); i += 2) {
            const auto& left = layer[i];
            const auto& right = i + 1 < layer.size() ? layer[i + 1] : left;
            std::vector<uint8_t> pair;
            pair.reserve(64);
            pair.insert(pair.end(), left.begin(), left.end());
            pair.insert(pair.end(), right.begin(), right.end());
            next.push_back(crypto::sha256(pair));
        }
        layer = std::move(next);
    }
    return layer.front();
}

std::array<uint8_t, 32> Block::hash() const
{
    return header.hash();
}

std::vector<uint8_t> Block::serialize() const
{
    if (transactions.size() > MAX_TRANSACTION_COUNT)
        throw std::runtime_error("block: too many transactions");

    const auto headerBytes = header.toBytes();
    std::vector<uint8_t> out;
    out.reserve(headerBytes.size() + 16);
    out.insert(out.end(), BLOCK_MAGIC.begin(), BLOCK_MAGIC.end());
    writeU16(out, BLOCK_VERSION);
    writeU32(out, static_cast<uint32_t>(headerBytes.size()));
    out.insert(out.end(), headerBytes.begin(), headerBytes.end());
    writeU32(out, static_cast<uint32_t>(transactions.size()));

    for (const auto& tx : transactions) {
        const auto txBytes = tx.serialize();
        writeU32(out, static_cast<uint32_t>(txBytes.size()));
        out.insert(out.end(), txBytes.begin(), txBytes.end());
        if (out.size() > MAX_BLOCK_SIZE)
            throw std::runtime_error("block: serialized block too large");
    }
    return out;
}

bool Block::deserialize(const std::vector<uint8_t>& data,
                        Block& out,
                        std::string& error)
{
    if (data.size() > MAX_BLOCK_SIZE) {
        error = "block: payload too large";
        return false;
    }
    if (data.size() < 4 + 2 + 4 + 4) {
        error = "block: truncated header";
        return false;
    }

    std::size_t offset = 0;
    if (!std::equal(BLOCK_MAGIC.begin(), BLOCK_MAGIC.end(), data.begin())) {
        error = "block: invalid magic";
        return false;
    }
    offset += BLOCK_MAGIC.size();

    uint16_t version = 0;
    uint32_t headerSize = 0;
    if (!readU16(data, offset, version) || version != BLOCK_VERSION) {
        error = "block: unsupported version";
        return false;
    }
    if (!readU32(data, offset, headerSize) || headerSize > data.size() - offset) {
        error = "block: invalid header length";
        return false;
    }

    std::vector<uint8_t> headerBytes(
        data.begin() + static_cast<std::ptrdiff_t>(offset),
        data.begin() + static_cast<std::ptrdiff_t>(offset + headerSize));
    offset += headerSize;

    Block decoded;
    if (!BlockHeader::deserialize(headerBytes, decoded.header, error))
        return false;

    uint32_t transactionCount = 0;
    if (!readU32(data, offset, transactionCount) ||
        transactionCount > MAX_TRANSACTION_COUNT) {
        error = "block: invalid transaction count";
        return false;
    }

    decoded.transactions.reserve(transactionCount);
    for (uint32_t i = 0; i < transactionCount; ++i) {
        uint32_t txSize = 0;
        if (!readU32(data, offset, txSize) ||
            txSize == 0 || txSize > data.size() - offset) {
            error = "block: invalid transaction length";
            return false;
        }

        std::vector<uint8_t> txBytes(
            data.begin() + static_cast<std::ptrdiff_t>(offset),
            data.begin() + static_cast<std::ptrdiff_t>(offset + txSize));
        offset += txSize;

        Transaction tx;
        if (!Transaction::deserialize(txBytes, tx, error))
            return false;
        decoded.transactions.push_back(std::move(tx));
    }

    if (offset != data.size()) {
        error = "block: trailing bytes";
        return false;
    }

    out = std::move(decoded);
    return true;
}

Block Block::deserialize(const std::vector<uint8_t>& data)
{
    Block out;
    std::string error;
    if (!deserialize(data, out, error))
        throw std::runtime_error(error);
    return out;
}
