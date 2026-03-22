#include "core/header.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "core/shake256.h"

namespace colossusx {
namespace {

constexpr char kHeaderDigestDomain[] = "colossusx:header_digest:v1";

void append_u32_le(std::uint32_t value, std::vector<std::uint8_t>* out) {
  for (std::size_t i = 0; i < sizeof(value); ++i) {
    out->push_back(static_cast<std::uint8_t>((value >> (8U * i)) & 0xffU));
  }
}

void append_u64_le(std::uint64_t value, std::vector<std::uint8_t>* out) {
  for (std::size_t i = 0; i < sizeof(value); ++i) {
    out->push_back(static_cast<std::uint8_t>((value >> (8U * i)) & 0xffU));
  }
}

void append_bytes(const std::uint8_t* data, std::size_t size,
                  std::vector<std::uint8_t>* out) {
  out->insert(out->end(), data, data + size);
}

Hash256 vector_to_hash256(const std::vector<std::uint8_t>& input) {
  if (input.size() != kHashBytes) {
    throw std::invalid_argument("expected 32-byte digest");
  }
  Hash256 output{};
  for (std::size_t i = 0; i < kHashBytes; ++i) {
    output[i] = input[i];
  }
  return output;
}

std::string bytes_to_hex(const std::uint8_t* data, std::size_t size) {
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (std::size_t i = 0; i < size; ++i) {
    stream << std::setw(2) << static_cast<unsigned>(data[i]);
  }
  return stream.str();
}

}  // namespace

std::vector<std::uint8_t> canonical_header_serialization(const BlockHeader& header) {
  std::vector<std::uint8_t> serialized;
  serialized.reserve(4 + 8 + 8 + 4 + 32 + 32 + 32);
  append_u32_le(header.version, &serialized);
  append_u64_le(header.height, &serialized);
  append_u64_le(header.timestamp, &serialized);
  append_u32_le(header.bits, &serialized);
  append_bytes(header.previous_block_hash.data(), header.previous_block_hash.size(), &serialized);
  append_bytes(header.transactions_root.data(), header.transactions_root.size(), &serialized);
  append_bytes(header.state_root.data(), header.state_root.size(), &serialized);
  return serialized;
}

Hash256 header_digest(const BlockHeader& header) {
  std::vector<std::uint8_t> input;
  const std::size_t domain_size = sizeof(kHeaderDigestDomain) - 1;
  const std::vector<std::uint8_t> serialized = canonical_header_serialization(header);
  input.reserve(domain_size + 1 + serialized.size());
  append_bytes(reinterpret_cast<const std::uint8_t*>(kHeaderDigestDomain), domain_size, &input);
  input.push_back(0x00U);
  input.insert(input.end(), serialized.begin(), serialized.end());
  return vector_to_hash256(shake256(input, kHashBytes));
}

std::uint64_t epoch_index_from_height(std::uint64_t height) {
  return height / kEpochLengthBlocks;
}

std::string to_hex(const std::vector<std::uint8_t>& bytes) {
  return bytes_to_hex(bytes.data(), bytes.size());
}

std::string to_hex(const Hash256& bytes) { return bytes_to_hex(bytes.data(), bytes.size()); }

}  // namespace colossusx
