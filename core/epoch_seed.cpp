#include "core/epoch_seed.h"

#include <stdexcept>
#include <vector>

#include "core/shake256.h"

namespace colossusx {
namespace {

constexpr char kEpochSeedDomain[] = "colossusx:epoch_seed:v1";

void append_u64_le(std::uint64_t value, std::vector<std::uint8_t>* out) {
  for (std::size_t i = 0; i < sizeof(value); ++i) {
    out->push_back(static_cast<std::uint8_t>((value >> (8U * i)) & 0xffU));
  }
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

}  // namespace

Hash256 epoch_seed(const ChainId& chain_id,
                   std::uint64_t epoch_index,
                   const Hash256& prev_epoch_anchor_hash) {
  std::vector<std::uint8_t> input;
  const std::size_t domain_size = sizeof(kEpochSeedDomain) - 1;
  input.reserve(domain_size + 1 + chain_id.size() + sizeof(epoch_index) + prev_epoch_anchor_hash.size());
  input.insert(input.end(), kEpochSeedDomain, kEpochSeedDomain + domain_size);
  input.push_back(0x00U);
  input.insert(input.end(), chain_id.begin(), chain_id.end());
  append_u64_le(epoch_index, &input);
  input.insert(input.end(), prev_epoch_anchor_hash.begin(), prev_epoch_anchor_hash.end());
  return vector_to_hash256(shake256(input, kHashBytes));
}

}  // namespace colossusx
