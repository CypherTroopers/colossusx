#include "tools/generate_test_vectors.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/epoch_cache.h"
#include "core/epoch_seed.h"
#include "core/get_item.h"
#include "core/header.h"
#include "core/pow.h"
#include "core/round_function.h"
#include "core/scratch.h"
#include "core/shake256.h"
#include "core/verifier.h"

namespace colossusx {
namespace vectors {
namespace {

using RoundVectorCase = std::tuple<std::string, Profile, Hash256, Hash256, std::uint64_t, std::uint32_t, std::uint64_t>;
using ReplayVectorCase = std::tuple<std::string, Profile, Hash256, Hash256, std::uint64_t>;

std::string bytes_to_hex(const std::uint8_t* data, std::size_t size) {
  static constexpr char kHexDigits[] = "0123456789abcdef";
  std::string hex;
  hex.reserve(size * 2U);
  for (std::size_t i = 0; i < size; ++i) {
    hex.push_back(kHexDigits[(data[i] >> 4U) & 0x0fU]);
    hex.push_back(kHexDigits[data[i] & 0x0fU]);
  }
  return hex;
}

template <typename Container>
std::string container_hex(const Container& bytes) {
  return bytes_to_hex(bytes.data(), bytes.size());
}

Hash256 make_hash(std::uint8_t start) {
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<std::uint8_t>(start + i);
  }
  return hash;
}

ChainId make_chain_id(std::uint8_t start) {
  ChainId chain_id{};
  for (std::size_t i = 0; i < chain_id.size(); ++i) {
    chain_id[i] = static_cast<std::uint8_t>(start + i);
  }
  return chain_id;
}

BlockHeader make_header(std::uint32_t version,
                        std::uint64_t height,
                        std::uint64_t timestamp,
                        std::uint32_t bits,
                        std::uint8_t hash_seed) {
  BlockHeader header;
  header.version = version;
  header.height = height;
  header.timestamp = timestamp;
  header.bits = bits;
  header.previous_block_hash = make_hash(hash_seed);
  header.transactions_root = make_hash(static_cast<std::uint8_t>(hash_seed + 0x20U));
  header.state_root = make_hash(static_cast<std::uint8_t>(hash_seed + 0x40U));
  header.nonce = 0;
  header.pow_result = make_hash(static_cast<std::uint8_t>(hash_seed + 0x60U));
  return header;
}

void append_line(const std::string& line, std::ostringstream* out) {
  *out << line << '\n';
}

void append_field(const std::string& name, const std::string& value, std::ostringstream* out) {
  append_line(name + ": " + value, out);
}

std::string u64_string(std::uint64_t value) {
  return std::to_string(value);
}

std::string profile_name(Profile profile) {
  return profile_parameters(profile).name;
}

std::string hash_bytes_hex(const std::vector<std::uint8_t>& bytes) {
  const std::vector<std::uint8_t> digest = shake256(bytes, kHashBytes);
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = digest[i];
  }
  return to_hex(hash);
}

std::string hash_dataset_item(const DatasetItem& item) {
  return hash_bytes_hex(std::vector<std::uint8_t>(item.begin(), item.end()));
}

std::string hash_lane_field(const std::vector<LaneReplayResult>& lanes, bool state_x) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(lanes.size() * 4U * sizeof(std::uint64_t));
  for (const LaneReplayResult& lane : lanes) {
    const auto& words = state_x ? lane.final_state.x : lane.final_state.a;
    for (std::uint64_t word : words) {
      for (std::size_t i = 0; i < sizeof(word); ++i) {
        bytes.push_back(static_cast<std::uint8_t>((word >> (8U * i)) & 0xffU));
      }
    }
  }
  return hash_bytes_hex(bytes);
}

std::string word_array_hex(const std::array<std::uint64_t, 4>& words) {
  std::vector<std::uint8_t> bytes(words.size() * sizeof(std::uint64_t));
  for (std::size_t i = 0; i < words.size(); ++i) {
    for (std::size_t j = 0; j < sizeof(std::uint64_t); ++j) {
      bytes[i * sizeof(std::uint64_t) + j] =
          static_cast<std::uint8_t>((words[i] >> (8U * j)) & 0xffU);
    }
  }
  return container_hex(bytes);
}

std::string unit_pair_hex(const ScratchUnit& lhs, const ScratchUnit& rhs) {
  return container_hex(lhs) + "|" + container_hex(rhs);
}

std::string join_offsets(const std::vector<std::uint64_t>& offsets) {
  std::ostringstream out;
  for (std::size_t i = 0; i < offsets.size(); ++i) {
    if (i != 0U) {
      out << ',';
    }
    out << offsets[i];
  }
  return out.str();
}

std::string join_hex_strings(const std::vector<std::string>& values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0U) {
      out << '|';
    }
    out << values[i];
  }
  return out.str();
}

std::vector<std::uint64_t> default_cache_offsets(Profile profile) {
  const std::uint64_t cache_size = profile_parameters(profile).cache_size_bytes;
  return {0U, 4096U, cache_size / 2U, cache_size - 64U};
}

void append_separator(std::ostringstream* out) {
  *out << '\n';
}

void append_tv001(VectorPreset preset, std::ostringstream* out) {
  struct Case {
    std::string suffix;
    ChainId chain_id;
    std::uint64_t epoch_index;
    Hash256 prev_anchor;
    std::string notes;
  };
  std::vector<Case> cases = {
      {"A", make_chain_id(0x01U), 1U, make_hash(0x10U), "minimum nonzero epoch index"},
      {"B", make_chain_id(0x01U), 123U, make_hash(0x20U), "mid-range epoch index"},
      {"C", make_chain_id(0x01U), 1ULL << 20U, make_hash(0x30U), "large epoch index"},
      {"D", make_chain_id(0x11U), 123U, make_hash(0x20U), "different chain_id"},
  };
  if (preset == VectorPreset::kSmoke) {
    cases.resize(1U);
  }

  for (const Case& test_case : cases) {
    append_line("## TV-001-" + test_case.suffix, out);
    append_field("vector_id", "TV-001-" + test_case.suffix, out);
    append_field("chain_id", container_hex(test_case.chain_id), out);
    append_field("epoch_index", u64_string(test_case.epoch_index), out);
    append_field("prev_epoch_anchor_hash", to_hex(test_case.prev_anchor), out);
    append_field("expected_epoch_seed",
                 to_hex(epoch_seed(test_case.chain_id, test_case.epoch_index, test_case.prev_anchor)), out);
    append_field("notes", test_case.notes, out);
    append_separator(out);
  }
}

void append_tv002(VectorPreset preset, std::ostringstream* out) {
  BlockHeader minimal = make_header(1U, 0U, 0U, 0x1d00ffffU, 0x10U);
  BlockHeader nontrivial = make_header(2U, 4321U, 1717171717ULL, 0x1c654657U, 0x21U);
  BlockHeader one_field = nontrivial;
  one_field.timestamp += 1U;
  BlockHeader nonce_exclusion = nontrivial;
  nonce_exclusion.nonce = 99U;
  nonce_exclusion.pow_result = make_hash(0xa0U);

  struct Case {
    std::string suffix;
    std::string description;
    BlockHeader header;
    std::string notes;
  };
  std::vector<Case> cases = {
      {"A", "minimal valid header", minimal, "minimal case"},
      {"B", "header with nontrivial field values", nontrivial, "nontrivial case"},
      {"C", "header differing in timestamp only", one_field, "one-field-different case"},
      {"D", "nonce exclusion case", nonce_exclusion, "proves nonce and pow_result exclusion"},
  };
  if (preset == VectorPreset::kSmoke) {
    cases.resize(1U);
  }

  for (const Case& test_case : cases) {
    append_line("## TV-002-" + test_case.suffix, out);
    append_field("vector_id", "TV-002-" + test_case.suffix, out);
    append_field("header_description", test_case.description, out);
    append_field("serialized_header_without_nonce", to_hex(canonical_header_serialization(test_case.header)), out);
    append_field("expected_header_digest", to_hex(header_digest(test_case.header)), out);
    append_field("notes", test_case.notes, out);
    append_separator(out);
  }
}

void append_tv003(VectorPreset preset, std::ostringstream* out) {
  struct Case {
    std::string suffix;
    Profile profile;
    Hash256 epoch_seed_value;
    std::vector<std::uint64_t> offsets;
  };
  std::vector<Case> cases = {
      {"A", Profile::kCx18, make_hash(0x31U), default_cache_offsets(Profile::kCx18)},
      {"B", Profile::kCx32, make_hash(0x41U), default_cache_offsets(Profile::kCx32)},
      {"C", Profile::kCx18, make_hash(0x51U), {128U, 65536U, (256ULL << 20U) - 128U}},
  };
  if (preset == VectorPreset::kSmoke) {
    cases.resize(1U);
  }

  for (const Case& test_case : cases) {
    std::vector<std::string> samples;
    for (std::uint64_t offset : test_case.offsets) {
      samples.push_back(to_hex(epoch_cache_slice(test_case.profile, test_case.epoch_seed_value, offset, 64U)));
    }
    const EpochCache cache = build_epoch_cache(test_case.profile, test_case.epoch_seed_value);

    append_line("## TV-003-" + test_case.suffix, out);
    append_field("vector_id", "TV-003-" + test_case.suffix, out);
    append_field("profile", profile_name(test_case.profile), out);
    append_field("epoch_seed", to_hex(test_case.epoch_seed_value), out);
    append_field("cache_size", u64_string(profile_parameters(test_case.profile).cache_size_bytes), out);
    append_field("cache_sample_offsets", join_offsets(test_case.offsets), out);
    append_field("expected_cache_sample_hex", join_hex_strings(samples), out);
    append_field("expected_cache_hash", hash_bytes_hex(cache), out);
    append_separator(out);
  }
}

void append_tv004(VectorPreset preset, std::ostringstream* out) {
  struct Case {
    std::string suffix;
    Profile profile;
    Hash256 epoch_seed_value;
    std::uint64_t item_index;
    std::string notes;
  };
  const std::uint64_t cx18_count = dataset_item_count(Profile::kCx18);
  std::vector<Case> cases = {
      {"A", Profile::kCx18, make_hash(0x61U), 0U, "first dataset item"},
      {"B", Profile::kCx18, make_hash(0x61U), 17U, "low index item"},
      {"C", Profile::kCx18, make_hash(0x61U), cx18_count / 2U, "mid-range item"},
      {"D", Profile::kCx18, make_hash(0x61U), cx18_count - 1U, "high index item near dataset end"},
      {"E", Profile::kCx32, make_hash(0x71U), 17U, "same index under another profile"},
  };
  if (preset == VectorPreset::kSmoke) {
    cases.resize(1U);
  }

  for (const Case& test_case : cases) {
    append_line("## TV-004-" + test_case.suffix, out);
    append_field("vector_id", "TV-004-" + test_case.suffix, out);
    append_field("profile", profile_name(test_case.profile), out);
    append_field("epoch_seed", to_hex(test_case.epoch_seed_value), out);
    append_field("item_index", u64_string(test_case.item_index), out);
    append_field("expected_item_hex", container_hex(get_item(test_case.profile, test_case.epoch_seed_value, test_case.item_index)), out);
    append_field("notes", test_case.notes, out);
    append_separator(out);
  }
}

void append_tv005(VectorPreset preset, std::ostringstream* out) {
  struct Case {
    std::string suffix;
    Profile profile;
    Hash256 epoch_seed_value;
    Hash256 header_digest_value;
    std::uint64_t nonce;
    std::uint32_t lane_id;
  };
  std::vector<Case> cases = {
      {"A", Profile::kCx18, make_hash(0x81U), make_hash(0x91U), 42U, 0U},
      {"B", Profile::kCx18, make_hash(0x81U), make_hash(0x91U), 42U, 7U},
      {"C", Profile::kCx32, make_hash(0xa1U), make_hash(0xb1U), 7U, 1U},
  };
  if (preset == VectorPreset::kSmoke) {
    cases.resize(1U);
  }

  for (const Case& test_case : cases) {
    const ScratchUnit unit = derive_scratch_unit(test_case.profile, test_case.epoch_seed_value,
                                                 test_case.header_digest_value, test_case.nonce,
                                                 test_case.lane_id, 0U);
    const LaneScratch scratch = LaneScratch::Initialize(test_case.profile, test_case.epoch_seed_value,
                                                        test_case.header_digest_value, test_case.nonce,
                                                        test_case.lane_id);
    append_line("## TV-005-" + test_case.suffix, out);
    append_field("vector_id", "TV-005-" + test_case.suffix, out);
    append_field("profile", profile_name(test_case.profile), out);
    append_field("epoch_seed", to_hex(test_case.epoch_seed_value), out);
    append_field("header_digest", to_hex(test_case.header_digest_value), out);
    append_field("nonce", u64_string(test_case.nonce), out);
    append_field("lane_id", u64_string(test_case.lane_id), out);
    append_field("expected_scratch_prefix_hex", container_hex(unit), out);
    append_field("expected_scratch_hash", to_hex(scratch_integrity_hash(scratch)), out);
    append_separator(out);
  }
}

void append_tv006(VectorPreset preset, std::ostringstream* out) {
  std::vector<RoundVectorCase> cases = {
      {"A", Profile::kCx18, make_hash(0xc1U), make_hash(0xd1U), 5U, 0U, 0U},
      {"B", Profile::kCx18, make_hash(0xc1U), make_hash(0xd1U), 5U, 3U, 17U},
      {"C", Profile::kCx32, make_hash(0xe1U), make_hash(0xf1U), 11U, 1U, 0U},
  };
  if (preset == VectorPreset::kSmoke) {
    cases.resize(1U);
  }

  for (const RoundVectorCase& test_case : cases) {
    const Profile profile = std::get<1>(test_case);
    const Hash256& epoch_seed_value = std::get<2>(test_case);
    const Hash256& header_digest_value = std::get<3>(test_case);
    const std::uint64_t nonce = std::get<4>(test_case);
    const std::uint32_t lane_id = std::get<5>(test_case);
    const std::uint64_t round_index = std::get<6>(test_case);

    LaneScratch scratch = LaneScratch::Initialize(profile, epoch_seed_value, header_digest_value, nonce, lane_id);
    const LaneState initial_state = initialize_lane_state(profile, epoch_seed_value, header_digest_value, nonce, lane_id);
    const RoundTrace trace = execute_round(profile, epoch_seed_value, round_index, initial_state, &scratch);

    LaneScratch initial_scratch = LaneScratch::Initialize(profile, epoch_seed_value, header_digest_value, nonce, lane_id);
    const ScratchUnit s0 = initial_scratch.ReadUnit(trace.scratch_read_index0);
    const ScratchUnit s1 = initial_scratch.ReadUnit(trace.scratch_read_index1);
    const DatasetItem d0 = get_item(profile, epoch_seed_value, trace.item_index0);
    const DatasetItem d1 = get_item(profile, epoch_seed_value, trace.item_index1);
    const DatasetItem d2 = get_item(profile, epoch_seed_value, trace.item_index2);

    const std::string suffix = std::get<0>(test_case);
    append_line("## TV-006-" + suffix, out);
    append_field("vector_id", "TV-006-" + suffix, out);
    append_field("profile", profile_name(profile), out);
    append_field("epoch_seed", to_hex(epoch_seed_value), out);
    append_field("header_digest", to_hex(header_digest_value), out);
    append_field("nonce", u64_string(nonce), out);
    append_field("lane_id", u64_string(lane_id), out);
    append_field("round_index", u64_string(round_index), out);
    append_field("initial_state_x", word_array_hex(initial_state.x), out);
    append_field("initial_state_a", word_array_hex(initial_state.a), out);
    append_field("initial_scratch_reads", unit_pair_hex(s0, s1), out);
    append_field("expected_i0", u64_string(trace.item_index0), out);
    append_field("expected_i1", u64_string(trace.item_index1), out);
    append_field("expected_i2", u64_string(trace.item_index2), out);
    append_field("expected_d0_hash", hash_dataset_item(d0), out);
    append_field("expected_d1_hash", hash_dataset_item(d1), out);
    append_field("expected_d2_hash", hash_dataset_item(d2), out);
    append_field("expected_write_index", u64_string(trace.scratch_write_index), out);
    append_field("expected_written_value", container_hex(trace.scratch_write_unit), out);
    append_field("expected_next_state_x", word_array_hex(trace.next_state.x), out);
    append_field("expected_next_state_a", word_array_hex(trace.next_state.a), out);
    append_separator(out);
  }
}

Hash256 append_tv007(VectorPreset preset, std::ostringstream* out) {
  std::vector<ReplayVectorCase> cases = {
      {"A", Profile::kCx18, make_hash(0x11U), make_hash(0x21U), 0x0123456789abcdefULL},
      {"B", Profile::kCx32, make_hash(0x31U), make_hash(0x41U), 0x1111222233334444ULL},
      {"C", Profile::kCx64, make_hash(0x51U), make_hash(0x61U), 0x5555666677778888ULL},
      {"D", Profile::kCx128, make_hash(0x71U), make_hash(0x81U), 0x9999aaaabbbbccccULL},
      {"E", Profile::kCx18, make_hash(0x11U), make_hash(0x21U), 0x0123456789abcdf0ULL},
  };
  if (preset == VectorPreset::kSmoke) {
    cases.resize(1U);
  }

  Hash256 first_pow_hash{};
  bool first_hash_set = false;
  for (const ReplayVectorCase& test_case : cases) {
    const std::string suffix = std::get<0>(test_case);
    const Profile profile = std::get<1>(test_case);
    const Hash256& epoch_seed_value = std::get<2>(test_case);
    const Hash256& header_digest_value = std::get<3>(test_case);
    const std::uint64_t nonce = std::get<4>(test_case);
    const std::vector<LaneReplayResult> lanes = replay_nonce(profile, epoch_seed_value, header_digest_value, nonce);

    append_line("## TV-007-" + suffix, out);
    append_field("vector_id", "TV-007-" + suffix, out);
    append_field("profile", profile_name(profile), out);
    append_field("epoch_seed", to_hex(epoch_seed_value), out);
    append_field("header_digest", to_hex(header_digest_value), out);
    append_field("nonce", u64_string(nonce), out);
    append_field("expected_final_lane_state_hash", hash_lane_field(lanes, true), out);
    const Hash256 replay_pow_hash = compute_pow_hash(profile, fold_lane_results(profile, lanes));
    append_field("expected_final_lane_acc_hash", hash_lane_field(lanes, false), out);
    append_field("expected_pow_hash", to_hex(replay_pow_hash), out);
    append_separator(out);
    if (!first_hash_set) {
      first_pow_hash = replay_pow_hash;
      first_hash_set = true;
    }
  }
  return first_pow_hash;
}

void append_tv008(VectorPreset preset, const Hash256& reference_hash, std::ostringstream* out) {
  Hash256 less_target = reference_hash;
  for (std::size_t i = 0; i < less_target.size(); ++i) {
    const std::uint16_t next = static_cast<std::uint16_t>(less_target[i]) + 1U;
    less_target[i] = static_cast<std::uint8_t>(next & 0xffU);
    if ((next & 0x100U) == 0U) {
      break;
    }
  }
  Hash256 zero_hash{};
  Hash256 max_hash{};
  max_hash.fill(0xffU);
  Hash256 near_zero_target{};
  near_zero_target[0] = 1U;

  struct Case {
    std::string suffix;
    Hash256 pow_hash_value;
    Hash256 target;
  };
  std::vector<Case> cases = {
      {"A", reference_hash, less_target},
      {"B", reference_hash, reference_hash},
      {"C", reference_hash, zero_hash},
      {"D", zero_hash, near_zero_target},
      {"E", max_hash, max_hash},
  };
  if (preset == VectorPreset::kSmoke) {
    cases.resize(1U);
  }

  for (const Case& test_case : cases) {
    append_line("## TV-008-" + test_case.suffix, out);
    append_field("vector_id", "TV-008-" + test_case.suffix, out);
    append_field("pow_hash", to_hex(test_case.pow_hash_value), out);
    append_field("target", to_hex(test_case.target), out);
    append_field("expected_valid", pow_hash_meets_target(test_case.pow_hash_value, test_case.target) ? "true" : "false", out);
    append_separator(out);
  }
}

}  // namespace

std::string generate_test_vectors_document(VectorPreset preset) {
  std::ostringstream out;
  append_tv001(preset, &out);
  append_tv002(preset, &out);
  append_tv003(preset, &out);
  append_tv004(preset, &out);
  append_tv005(preset, &out);
  append_tv006(preset, &out);
  const Hash256 reference_hash = append_tv007(preset, &out);
  append_tv008(preset, reference_hash, &out);
  return out.str();
}

}  // namespace vectors
}  // namespace colossusx
