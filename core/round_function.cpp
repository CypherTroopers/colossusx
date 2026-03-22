#include "core/round_function.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace colossusx {
namespace {

constexpr std::uint64_t kMixConstant0 = 0x9e3779b97f4a7c15ULL;
constexpr std::uint64_t kMixConstant1 = 0xd6e8feb86659fd93ULL;
constexpr std::uint64_t kMixConstant2 = 0xa0761d6478bd642fULL;
constexpr std::uint64_t kMixConstant3 = 0xe7037ed1a0b428dbULL;

std::uint64_t rotate_left(std::uint64_t value, unsigned shift) {
  return shift == 0U ? value : ((value << shift) | (value >> (64U - shift)));
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

std::array<std::uint64_t, 4> item_digest_words(const DatasetItem& item) {
  std::array<std::uint64_t, 4> digest{};
  for (std::size_t group = 0; group < digest.size(); ++group) {
    std::uint64_t accumulator = 0;
    for (std::size_t offset = 0; offset < 8; ++offset) {
      const std::size_t word_index = group * 8 + offset;
      accumulator ^= rotate_left(load_u64_le(&item[word_index * sizeof(std::uint64_t)]),
                                 static_cast<unsigned>((group * 11U + offset * 7U) & 63U));
      accumulator += static_cast<std::uint64_t>(word_index) * kMixConstant0;
    }
    digest[group] = accumulator;
  }
  return digest;
}

std::array<std::uint64_t, 4> scratch_digest_words(const ScratchUnit& unit) {
  std::array<std::uint64_t, 4> digest{};
  for (std::size_t group = 0; group < digest.size(); ++group) {
    std::uint64_t accumulator = 0;
    for (std::size_t offset = 0; offset < 4; ++offset) {
      const std::size_t word_index = group * 4 + offset;
      accumulator ^= rotate_left(load_u64_le(&unit[word_index * sizeof(std::uint64_t)]),
                                 static_cast<unsigned>((group * 13U + offset * 9U) & 63U));
      accumulator += static_cast<std::uint64_t>(word_index) * kMixConstant1;
    }
    digest[group] = accumulator;
  }
  return digest;
}

std::uint64_t mix_word(std::uint64_t x,
                       std::uint64_t y,
                       std::uint64_t z,
                       std::uint64_t tweak) {
  std::uint64_t value = x ^ rotate_left(y + tweak, 17U);
  value += z ^ rotate_left(tweak, 41U);
  value *= (kMixConstant1 | 1ULL);
  value ^= rotate_left(value + kMixConstant2, 23U);
  return value;
}

}  // namespace

RoundTrace execute_round(Profile profile,
                         const Hash256& epoch_seed,
                         std::uint64_t round_index,
                         const LaneState& lane_state,
                         LaneScratch* scratch) {
  const std::uint64_t item_count = dataset_item_count(profile);
  const std::uint64_t scratch_units = scratch->unit_count();

  RoundTrace trace;
  trace.scratch_read_index0 =
      (lane_state.x[0] ^ rotate_left(lane_state.a[0], 7U) ^
       (round_index * kMixConstant0)) % scratch_units;
  trace.scratch_read_index1 =
      (lane_state.x[1] + rotate_left(lane_state.a[1], 11U) +
       (round_index * kMixConstant1) + trace.scratch_read_index0) % scratch_units;

  const ScratchUnit s0 = scratch->ReadUnit(trace.scratch_read_index0);
  const ScratchUnit s1 = scratch->ReadUnit(trace.scratch_read_index1);
  const std::array<std::uint64_t, 4> s0_words = scratch_digest_words(s0);
  const std::array<std::uint64_t, 4> s1_words = scratch_digest_words(s1);

  trace.item_index0 =
      (lane_state.x[0] + rotate_left(lane_state.a[2], 5U) +
       (round_index * kMixConstant2)) % item_count;
  const DatasetItem d0 = get_item(profile, epoch_seed, trace.item_index0);
  const std::array<std::uint64_t, 4> d0_words = item_digest_words(d0);

  trace.item_index1 =
      (lane_state.x[1] ^ d0_words[0] ^ rotate_left(s0_words[1], 19U) ^
       lane_state.a[3] ^ (round_index * kMixConstant3)) % item_count;
  const DatasetItem d1 = get_item(profile, epoch_seed, trace.item_index1);
  const std::array<std::uint64_t, 4> d1_words = item_digest_words(d1);

  trace.item_index2 =
      (lane_state.x[2] + d0_words[2] + rotate_left(d1_words[1], 17U) +
       s1_words[3] + lane_state.a[0]) % item_count;
  const DatasetItem d2 = get_item(profile, epoch_seed, trace.item_index2);
  const std::array<std::uint64_t, 4> d2_words = item_digest_words(d2);

  trace.t0 = mix_word(d0_words[0] ^ s0_words[0], d1_words[1], d2_words[2], lane_state.x[0] + round_index);
  trace.t1 = mix_word(d1_words[0] ^ s1_words[1], d2_words[1], d0_words[2], lane_state.a[1] + round_index);
  trace.t2 = mix_word(d2_words[0] ^ s0_words[2], d0_words[1], d1_words[2], lane_state.x[3] + lane_state.a[3]);

  for (std::size_t i = 0; i < trace.next_state.x.size(); ++i) {
    trace.next_state.x[i] =
        lane_state.x[i] + rotate_left(trace.t0 ^ d0_words[i], static_cast<unsigned>((9U * i + 7U) & 63U)) +
        rotate_left(s1_words[i] ^ trace.t2, static_cast<unsigned>((13U + 3U * i) & 63U));
    trace.next_state.a[i] =
        lane_state.a[i] ^ rotate_left(trace.t1 + d1_words[i], static_cast<unsigned>((11U * i + 5U) & 63U)) ^
        rotate_left(d2_words[i] + s0_words[i], static_cast<unsigned>((17U + 2U * i) & 63U));
  }

  trace.scratch_write_index =
      (trace.next_state.x[0] ^ trace.next_state.a[2] ^
       rotate_left(trace.t2, 27U) ^ trace.scratch_read_index1) % scratch_units;

  for (std::size_t word = 0; word < (kScratchUnitBytes / sizeof(std::uint64_t)); ++word) {
    const std::size_t lane = word % 4;
    const std::uint64_t value =
        mix_word(trace.next_state.x[lane] ^ load_u64_le(&s0[word * sizeof(std::uint64_t)]),
                 trace.next_state.a[(lane + 1U) % 4] ^ load_u64_le(&s1[word * sizeof(std::uint64_t)]),
                 trace.t0 ^ trace.t1 ^ trace.t2,
                 round_index + static_cast<std::uint64_t>(word) * kMixConstant0);
    store_u64_le(value, &trace.scratch_write_unit[word * sizeof(std::uint64_t)]);
  }

  scratch->WriteUnit(trace.scratch_write_index, trace.scratch_write_unit);
  return trace;
}

}  // namespace colossusx
