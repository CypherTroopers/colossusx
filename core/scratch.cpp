#include "core/scratch.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "core/shake256.h"

namespace colossusx {
namespace {

constexpr char kScratchDomain[] = "colossusx:scratch:v1";
constexpr std::uint64_t kMixConstant0 = 0x9e3779b97f4a7c15ULL;
constexpr std::uint64_t kMixConstant1 = 0xd6e8feb86659fd93ULL;
constexpr std::uint64_t kMixConstant2 = 0xa0761d6478bd642fULL;
constexpr std::uint64_t kMixConstant3 = 0xe7037ed1a0b428dbULL;

std::uint64_t rotate_left(std::uint64_t value, unsigned shift) {
  return shift == 0U ? value : ((value << shift) | (value >> (64U - shift)));
}

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

std::uint64_t load_u64_le(const std::uint8_t* data) {
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < sizeof(value); ++i) {
    value |= static_cast<std::uint64_t>(data[i]) << (8U * i);
  }
  return value;
}

void store_u64_le(std::uint64_t value, std::uint8_t* out) {
  for (std::size_t i = 0; i < sizeof(value); ++i) {
    out[i] = static_cast<std::uint8_t>((value >> (8U * i)) & 0xffU);
  }
}

std::array<std::uint64_t, kScratchUnitBytes / sizeof(std::uint64_t)> scratch_root_words(
    const ProfileParameters& parameters,
    const Hash256& epoch_seed,
    const Hash256& header_digest,
    std::uint64_t nonce,
    std::uint32_t lane_id) {
  std::vector<std::uint8_t> input;
  constexpr std::size_t domain_size = sizeof(kScratchDomain) - 1;
  input.reserve(domain_size + 1 + 5 + epoch_seed.size() + header_digest.size() + 8 + 4 + 8 + 8);
  input.insert(input.end(), kScratchDomain, kScratchDomain + domain_size);
  input.push_back(0x00U);
  input.insert(input.end(), parameters.name, parameters.name + 5);
  append_u64_le(parameters.scratch_total_bytes, &input);
  append_u32_le(parameters.lanes, &input);
  append_u32_le(parameters.scratch_unit_bytes, &input);
  append_u64_le(nonce, &input);
  append_u32_le(lane_id, &input);
  input.insert(input.end(), epoch_seed.begin(), epoch_seed.end());
  input.insert(input.end(), header_digest.begin(), header_digest.end());

  const std::vector<std::uint8_t> expanded = shake256(input, kScratchUnitBytes);
  std::array<std::uint64_t, kScratchUnitBytes / sizeof(std::uint64_t)> words{};
  for (std::size_t i = 0; i < words.size(); ++i) {
    words[i] = load_u64_le(&expanded[i * sizeof(std::uint64_t)]);
  }
  return words;
}

std::size_t byte_offset_for_unit(std::uint64_t unit_index) {
  return static_cast<std::size_t>(unit_index * kScratchUnitBytes);
}

}  // namespace

std::uint64_t lane_scratch_bytes(Profile profile) {
  const ProfileParameters& parameters = profile_parameters(profile);
  if ((parameters.scratch_total_bytes % parameters.lanes) != 0U) {
    throw std::invalid_argument("scratch_total must divide evenly across lanes");
  }
  const std::uint64_t per_lane = parameters.scratch_total_bytes / parameters.lanes;
  if ((per_lane % parameters.scratch_unit_bytes) != 0U) {
    throw std::invalid_argument("per-lane scratch must be a multiple of scratch_unit");
  }
  if (parameters.scratch_unit_bytes != kScratchUnitBytes) {
    throw std::invalid_argument("profile scratch unit does not match reference scratch unit");
  }
  return per_lane;
}

std::uint64_t lane_scratch_unit_count(Profile profile) {
  return lane_scratch_bytes(profile) / kScratchUnitBytes;
}

ScratchUnit derive_scratch_unit(Profile profile,
                                const Hash256& epoch_seed,
                                const Hash256& header_digest,
                                std::uint64_t nonce,
                                std::uint32_t lane_id,
                                std::uint64_t unit_index) {
  const ProfileParameters& parameters = profile_parameters(profile);
  if (unit_index >= lane_scratch_unit_count(profile)) {
    throw std::out_of_range("scratch unit index exceeds per-lane scratch");
  }

  auto words = scratch_root_words(parameters, epoch_seed, header_digest, nonce, lane_id);
  std::uint64_t running = nonce ^ static_cast<std::uint64_t>(lane_id) ^
                          (unit_index * kMixConstant0) ^ lane_scratch_bytes(profile);

  for (std::size_t i = 0; i < words.size(); ++i) {
    words[i] ^= rotate_left(unit_index * kMixConstant1 + static_cast<std::uint64_t>(i) * kMixConstant2,
                            static_cast<unsigned>((7U * i + lane_id) & 63U));
  }

  for (std::size_t round = 0; round < 4; ++round) {
    for (std::size_t i = 0; i < words.size(); ++i) {
      const std::uint64_t left = words[(i + 1U) % words.size()];
      const std::uint64_t right = words[(i + 5U) % words.size()];
      const std::uint64_t tweak = running + (static_cast<std::uint64_t>(round) * kMixConstant2) +
                                  (static_cast<std::uint64_t>(i) * kMixConstant3);
      words[i] ^= rotate_left(left + tweak, static_cast<unsigned>((5U * i + 9U * round) & 63U));
      words[i] += right ^ rotate_left(tweak, static_cast<unsigned>((13U + i + round) & 63U));
      words[i] *= (kMixConstant1 | 1ULL);
    }
    running ^= words[round] + rotate_left(words[(round + 9U) % words.size()],
                                          static_cast<unsigned>((11U * round + lane_id) & 63U));
  }

  ScratchUnit unit{};
  for (std::size_t i = 0; i < words.size(); ++i) {
    store_u64_le(words[i], &unit[i * sizeof(std::uint64_t)]);
  }
  return unit;
}

LaneScratch::LaneScratch(Profile profile, std::uint32_t lane_id, std::vector<std::uint8_t> bytes)
    : profile_(profile), lane_id_(lane_id), bytes_(std::move(bytes)) {}

LaneScratch LaneScratch::Initialize(Profile profile,
                                    const Hash256& epoch_seed,
                                    const Hash256& header_digest,
                                    std::uint64_t nonce,
                                    std::uint32_t lane_id) {
  const std::uint64_t units = lane_scratch_unit_count(profile);
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(lane_scratch_bytes(profile)), 0U);
  for (std::uint64_t unit_index = 0; unit_index < units; ++unit_index) {
    const ScratchUnit unit = derive_scratch_unit(profile, epoch_seed, header_digest, nonce, lane_id, unit_index);
    const std::size_t offset = byte_offset_for_unit(unit_index);
    for (std::size_t i = 0; i < unit.size(); ++i) {
      bytes[offset + i] = unit[i];
    }
  }
  return LaneScratch(profile, lane_id, std::move(bytes));
}

std::uint64_t LaneScratch::unit_count() const {
  return static_cast<std::uint64_t>(bytes_.size() / kScratchUnitBytes);
}

ScratchUnit LaneScratch::ReadUnit(std::uint64_t unit_index) const {
  if (unit_index >= unit_count()) {
    throw std::out_of_range("scratch read exceeds per-lane scratch");
  }
  ScratchUnit unit{};
  const std::size_t offset = byte_offset_for_unit(unit_index);
  for (std::size_t i = 0; i < unit.size(); ++i) {
    unit[i] = bytes_[offset + i];
  }
  return unit;
}

void LaneScratch::WriteUnit(std::uint64_t unit_index, const ScratchUnit& unit) {
  if (unit_index >= unit_count()) {
    throw std::out_of_range("scratch write exceeds per-lane scratch");
  }
  const std::size_t offset = byte_offset_for_unit(unit_index);
  for (std::size_t i = 0; i < unit.size(); ++i) {
    bytes_[offset + i] = unit[i];
  }
}

Hash256 scratch_integrity_hash(const LaneScratch& scratch) {
  const std::vector<std::uint8_t> digest = shake256(scratch.bytes(), kHashBytes);
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = digest[i];
  }
  return hash;
}

}  // namespace colossusx
