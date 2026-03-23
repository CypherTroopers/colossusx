#include "core/get_item.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "core/shake256.h"

namespace colossusx {
namespace {

constexpr char kGetItemDomain[] = "colossusx:get_item:v1";
constexpr std::uint64_t kMixConstant0 = 0x9e3779b97f4a7c15ULL;
constexpr std::uint64_t kMixConstant1 = 0xd6e8feb86659fd93ULL;
constexpr std::uint64_t kMixConstant2 = 0xa0761d6478bd642fULL;
constexpr std::uint64_t kMixConstant3 = 0xe7037ed1a0b428dbULL;

std::uint64_t rotate_left(std::uint64_t value, unsigned shift) {
  return shift == 0U ? value : ((value << shift) | (value >> (64U - shift)));
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

std::array<std::uint64_t, kDatasetItemBytes / sizeof(std::uint64_t)> initial_item_state(
    const ProfileParameters& parameters,
    const Hash256& epoch_seed,
    std::uint64_t item_index) {
  std::vector<std::uint8_t> input;
  constexpr std::size_t domain_size = sizeof(kGetItemDomain) - 1;
  input.reserve(domain_size + 1 + 5 + epoch_seed.size() + 8 + 8 + 8);
  input.insert(input.end(), kGetItemDomain, kGetItemDomain + domain_size);
  input.push_back(0x00U);
  input.insert(input.end(), parameters.name, parameters.name + 5);
  append_u64_le(parameters.dataset_size_bytes, &input);
  append_u64_le(parameters.cache_size_bytes, &input);
  append_u64_le(item_index, &input);
  input.insert(input.end(), epoch_seed.begin(), epoch_seed.end());

  const std::vector<std::uint8_t> expanded = shake256(input, kDatasetItemBytes);
  std::array<std::uint64_t, kDatasetItemBytes / sizeof(std::uint64_t)> state{};
  for (std::size_t i = 0; i < state.size(); ++i) {
    state[i] = load_u64_le(&expanded[i * sizeof(std::uint64_t)]);
  }
  return state;
}

std::uint64_t load_cache_tap_word(Profile profile,
                                  const Hash256& epoch_seed,
                                  std::uint64_t cache_word_index) {
  return epoch_cache_word(profile, epoch_seed, cache_word_index);
}


struct ItemMemoEntry {
  bool initialized = false;
  Profile profile = Profile::kCx18;
  Hash256 epoch_seed{};
  std::unordered_map<std::uint64_t, DatasetItem> items;
};

ItemMemoEntry& item_memo() {
  static ItemMemoEntry memo;
  return memo;
}

}  // namespace

std::uint64_t dataset_item_count(Profile profile) {
  const ProfileParameters& parameters = profile_parameters(profile);
  if ((parameters.dataset_size_bytes % parameters.item_size_bytes) != 0U) {
    throw std::invalid_argument("dataset size must be divisible by item size");
  }
  return parameters.dataset_size_bytes / parameters.item_size_bytes;
}

DatasetItem get_item(Profile profile, const Hash256& epoch_seed, std::uint64_t item_index) {
  const bool use_memo = !epoch_cache_override_active(profile, epoch_seed);
  ItemMemoEntry& memo = item_memo();
  if (use_memo && (!memo.initialized || memo.profile != profile || memo.epoch_seed != epoch_seed)) {
    memo.initialized = true;
    memo.profile = profile;
    memo.epoch_seed = epoch_seed;
    memo.items.clear();
  }
  if (use_memo) {
    const auto found = memo.items.find(item_index);
    if (found != memo.items.end()) {
      return found->second;
    }
  }

  const ProfileParameters& parameters = profile_parameters(profile);
  if (parameters.item_size_bytes != kDatasetItemBytes) {
    throw std::invalid_argument("profile item size does not match reference item size");
  }

  const std::uint64_t item_count = dataset_item_count(profile);
  if (item_index >= item_count) {
    throw std::out_of_range("item index exceeds dataset size");
  }

  constexpr std::size_t kWordCount = kDatasetItemBytes / sizeof(std::uint64_t);
  auto state = initial_item_state(parameters, epoch_seed, item_index);
  const std::uint64_t cache_word_count = parameters.cache_size_bytes / sizeof(std::uint64_t);
  const std::uint64_t span = cache_word_count / parameters.get_item_taps;

  std::uint64_t running = item_index ^ parameters.dataset_size_bytes ^
                          rotate_left(parameters.cache_size_bytes, 9U);
  std::array<std::uint64_t, 2> perturbation = {state[3], state[17]};

  for (std::uint32_t tap = 0; tap < parameters.get_item_taps; ++tap) {
    const std::size_t lane = (tap * 7U) % kWordCount;
    const std::size_t neighbor = (lane + 11U) % kWordCount;
    const std::size_t tail = (lane + 23U) % kWordCount;

    std::uint64_t selector = state[lane] ^ rotate_left(state[neighbor], static_cast<unsigned>((tap + lane) & 63U)) ^
                             running ^ (static_cast<std::uint64_t>(tap) * kMixConstant0);
    if (tap >= 8U) {
      selector ^= rotate_left(perturbation[0], static_cast<unsigned>((tap + 13U) & 63U));
    }
    if (tap >= 16U) {
      selector += rotate_left(perturbation[1], static_cast<unsigned>((tap + 29U) & 63U));
    }

    const std::uint64_t base_word = (static_cast<std::uint64_t>(tap) * span) % cache_word_count;
    const std::uint64_t cache_word_index = (base_word + (selector % span)) % cache_word_count;
    const std::uint64_t tap_word = load_cache_tap_word(profile, epoch_seed, cache_word_index);

    state[lane] ^= rotate_left(tap_word + running + static_cast<std::uint64_t>(tap), static_cast<unsigned>((11U * tap + 5U) & 63U));
    state[neighbor] += tap_word ^ rotate_left(state[lane], static_cast<unsigned>((7U + tap) & 63U));
    state[tail] *= ((tap_word | 1ULL) ^ kMixConstant1);
    state[(lane + 5U) % kWordCount] += rotate_left(state[tail] ^ tap_word, static_cast<unsigned>((3U * tap + 17U) & 63U));

    running ^= tap_word + rotate_left(state[neighbor], static_cast<unsigned>((tap + 19U) & 63U));
    perturbation[static_cast<std::size_t>(tap >= 12U)] ^= tap_word + state[tail];
  }

  for (std::size_t round = 0; round < 4; ++round) {
    for (std::size_t i = 0; i < kWordCount; ++i) {
      const std::uint64_t left = state[(i + 1U) % kWordCount];
      const std::uint64_t right = state[(i + 9U) % kWordCount];
      const std::uint64_t tweak = running + (static_cast<std::uint64_t>(round) * kMixConstant2) +
                                  (static_cast<std::uint64_t>(i) * kMixConstant3);
      state[i] ^= rotate_left(left + tweak, static_cast<unsigned>((5U * i + 9U * round) & 63U));
      state[i] += right ^ rotate_left(tweak, static_cast<unsigned>((13U + i + round) & 63U));
      state[i] *= (kMixConstant0 | 1ULL);
    }
    running ^= state[round] + rotate_left(state[(round + 13U) % kWordCount], static_cast<unsigned>((round * 11U) & 63U));
  }

  DatasetItem item{};
  for (std::size_t i = 0; i < kWordCount; ++i) {
    store_u64_le(state[i], &item[i * sizeof(std::uint64_t)]);
  }
  if (use_memo) {
    memo.items.emplace(item_index, item);
  }
  return item;
}

}  // namespace colossusx
