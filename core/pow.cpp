#include "core/pow.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "core/shake256.h"

namespace colossusx {
namespace {

constexpr char kLaneInitDomain[] = "colossusx:lane_state:v1";
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

std::uint64_t mix_word(std::uint64_t x,
                       std::uint64_t y,
                       std::uint64_t z,
                       std::uint64_t tweak) {
  std::uint64_t value = x ^ rotate_left(y + tweak, 17U);
  value += z ^ rotate_left(tweak + kMixConstant0, 41U);
  value *= (kMixConstant1 | 1ULL);
  value ^= rotate_left(value + kMixConstant2, 23U);
  value += rotate_left(x ^ z, 31U);
  return value;
}

}  // namespace

LaneState initialize_lane_state(Profile profile,
                                const Hash256& epoch_seed,
                                const Hash256& header_digest,
                                std::uint64_t nonce,
                                std::uint32_t lane_id) {
  const ProfileParameters& parameters = profile_parameters(profile);

  std::vector<std::uint8_t> input;
  constexpr std::size_t domain_size = sizeof(kLaneInitDomain) - 1;
  input.reserve(domain_size + 1 + 5 + 8 + 4 + 4 + epoch_seed.size() + header_digest.size() + 8);
  input.insert(input.end(), kLaneInitDomain, kLaneInitDomain + domain_size);
  input.push_back(0x00U);
  input.insert(input.end(), parameters.name, parameters.name + 5);
  append_u64_le(parameters.rounds, &input);
  append_u32_le(parameters.lanes, &input);
  append_u32_le(lane_id, &input);
  append_u64_le(nonce, &input);
  input.insert(input.end(), epoch_seed.begin(), epoch_seed.end());
  input.insert(input.end(), header_digest.begin(), header_digest.end());

  const std::vector<std::uint8_t> expanded = shake256(input, 8U * sizeof(std::uint64_t));
  LaneState state;
  for (std::size_t i = 0; i < state.x.size(); ++i) {
    const std::uint64_t seed = load_u64_le(&expanded[i * sizeof(std::uint64_t)]);
    state.x[i] = seed ^ rotate_left(nonce + static_cast<std::uint64_t>(lane_id) * kMixConstant0,
                                    static_cast<unsigned>((9U * i + lane_id) & 63U));
  }
  for (std::size_t i = 0; i < state.a.size(); ++i) {
    const std::uint64_t seed = load_u64_le(&expanded[(i + 4U) * sizeof(std::uint64_t)]);
    state.a[i] = seed + rotate_left(parameters.dataset_size_bytes ^ epoch_seed[i],
                                    static_cast<unsigned>((11U * i + 7U + lane_id) & 63U));
  }

  for (std::size_t round = 0; round < 2; ++round) {
    for (std::size_t i = 0; i < state.x.size(); ++i) {
      const std::uint64_t tweak = nonce + static_cast<std::uint64_t>(lane_id) * kMixConstant1 +
                                  static_cast<std::uint64_t>(round) * kMixConstant2 +
                                  static_cast<std::uint64_t>(i) * kMixConstant3;
      state.x[i] = mix_word(state.x[i], state.a[(i + 1U) % 4], state.x[(i + 2U) % 4], tweak);
      state.a[i] = mix_word(state.a[i], state.x[(i + 3U) % 4], state.a[(i + 2U) % 4], tweak ^ kMixConstant0);
    }
  }

  return state;
}

LaneReplayResult replay_lane(Profile profile,
                             const Hash256& epoch_seed,
                             const Hash256& header_digest,
                             std::uint64_t nonce,
                             std::uint32_t lane_id) {
  const ProfileParameters& parameters = profile_parameters(profile);
  if (lane_id >= parameters.lanes) {
    throw std::out_of_range("lane id exceeds profile lane count");
  }

  LaneScratch scratch = LaneScratch::InitializeLazy(profile, epoch_seed, header_digest, nonce, lane_id);
  LaneState state = initialize_lane_state(profile, epoch_seed, header_digest, nonce, lane_id);
  for (std::uint32_t round = 0; round < parameters.rounds; ++round) {
    state = execute_round(profile, epoch_seed, round, state, &scratch).next_state;
  }

  LaneReplayResult result;
  result.lane_id = lane_id;
  result.final_state = state;
  return result;
}

std::vector<LaneReplayResult> replay_nonce(Profile profile,
                                           const Hash256& epoch_seed,
                                           const Hash256& header_digest,
                                           std::uint64_t nonce) {
  const ProfileParameters& parameters = profile_parameters(profile);
  std::vector<LaneReplayResult> results;
  results.reserve(parameters.lanes);
  for (std::uint32_t lane_id = 0; lane_id < parameters.lanes; ++lane_id) {
    results.push_back(replay_lane(profile, epoch_seed, header_digest, nonce, lane_id));
  }
  return results;
}

FoldedReplayState fold_lane_results(Profile profile,
                                    const std::vector<LaneReplayResult>& lane_results) {
  const ProfileParameters& parameters = profile_parameters(profile);
  if (lane_results.size() != parameters.lanes) {
    throw std::invalid_argument("lane result count must match profile lane count");
  }

  FoldedReplayState folded;
  folded.x = {kMixConstant0, kMixConstant1, kMixConstant2, kMixConstant3};
  folded.a = {parameters.rounds,
              parameters.dataset_size_bytes,
              parameters.cache_size_bytes,
              parameters.scratch_total_bytes};

  for (std::uint32_t expected_lane = 0; expected_lane < parameters.lanes; ++expected_lane) {
    if (lane_results[expected_lane].lane_id != expected_lane) {
      throw std::invalid_argument("lane results must be ordered by lane id");
    }
    const LaneState& lane = lane_results[expected_lane].final_state;
    for (std::size_t i = 0; i < folded.x.size(); ++i) {
      const std::uint64_t lane_bias = static_cast<std::uint64_t>(expected_lane + 1U) * kMixConstant0 +
                                      static_cast<std::uint64_t>(i) * kMixConstant2;
      folded.x[i] = mix_word(folded.x[i] ^ lane.x[i], lane.a[(i + 1U) % 4], folded.a[(i + 2U) % 4], lane_bias);
      folded.a[i] = mix_word(folded.a[i] + lane.a[i], lane.x[(i + 3U) % 4], folded.x[(i + 1U) % 4],
                             lane_bias ^ kMixConstant3);
    }
  }

  for (std::size_t i = 0; i < folded.x.size(); ++i) {
    folded.x[i] = mix_word(folded.x[i], folded.a[(i + 1U) % 4], folded.x[(i + 2U) % 4],
                           parameters.lanes + static_cast<std::uint64_t>(i) * kMixConstant1);
    folded.a[i] = mix_word(folded.a[i], folded.x[(i + 3U) % 4], folded.a[(i + 2U) % 4],
                           parameters.get_item_taps + static_cast<std::uint64_t>(i) * kMixConstant2);
  }

  return folded;
}

Hash256 compute_pow_hash(Profile profile,
                         const FoldedReplayState& folded_state) {
  const ProfileParameters& parameters = profile_parameters(profile);
  std::array<std::uint64_t, 4> digest = folded_state.x;
  std::array<std::uint64_t, 4> acc = folded_state.a;

  for (std::size_t round = 0; round < 4; ++round) {
    for (std::size_t i = 0; i < digest.size(); ++i) {
      const std::uint64_t tweak = parameters.rounds + parameters.dataset_size_bytes +
                                  static_cast<std::uint64_t>(round) * kMixConstant0 +
                                  static_cast<std::uint64_t>(i) * kMixConstant3;
      digest[i] = mix_word(digest[i], acc[(i + 1U) % 4], digest[(i + 2U) % 4], tweak);
      acc[i] = mix_word(acc[i], digest[(i + 3U) % 4], acc[(i + 2U) % 4], tweak ^ kMixConstant1);
    }
  }

  Hash256 hash{};
  for (std::size_t i = 0; i < digest.size(); ++i) {
    store_u64_le(digest[i] ^ rotate_left(acc[i], static_cast<unsigned>((i * 13U + 17U) & 63U)),
                 &hash[i * sizeof(std::uint64_t)]);
  }
  return hash;
}

Hash256 pow_hash(Profile profile,
                 const Hash256& epoch_seed,
                 const Hash256& header_digest,
                 std::uint64_t nonce) {
  return compute_pow_hash(profile, fold_lane_results(profile, replay_nonce(profile, epoch_seed, header_digest, nonce)));
}

}  // namespace colossusx
