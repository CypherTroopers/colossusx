#ifndef COLOSSUSX_CORE_HEADER_H_
#define COLOSSUSX_CORE_HEADER_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace colossusx {

constexpr std::size_t kHashBytes = 32;
constexpr std::size_t kChainIdBytes = 16;
constexpr std::uint64_t kTargetBlockTimeSeconds = 60;
constexpr std::uint64_t kTargetEpochWallTimeSeconds = 72U * 60U * 60U;
constexpr std::uint64_t kEpochLengthBlocks = 4320;

using Hash256 = std::array<std::uint8_t, kHashBytes>;
using ChainId = std::array<std::uint8_t, kChainIdBytes>;

struct BlockHeader {
  std::uint32_t version = 0;
  std::uint64_t height = 0;
  std::uint64_t timestamp = 0;
  std::uint32_t bits = 0;
  Hash256 previous_block_hash{};
  Hash256 transactions_root{};
  Hash256 state_root{};
  std::uint64_t nonce = 0;
  Hash256 pow_result{};
};

std::vector<std::uint8_t> canonical_header_serialization(const BlockHeader& header);
Hash256 header_digest(const BlockHeader& header);
std::uint64_t epoch_index_from_height(std::uint64_t height);
std::string to_hex(const std::vector<std::uint8_t>& bytes);
std::string to_hex(const Hash256& bytes);

}  // namespace colossusx

#endif
