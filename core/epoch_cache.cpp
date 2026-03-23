#include "core/epoch_cache.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

#include "core/shake256.h"

namespace colossusx {
namespace {

constexpr char kEpochCacheDomain[] = "colossusx:epoch_cache:v1";
constexpr std::size_t kEpochCacheBlockBytes = 64;
constexpr std::uint64_t kMixConstant0 = 0x9e3779b97f4a7c15ULL;
constexpr std::uint64_t kMixConstant1 = 0xd6e8feb86659fd93ULL;
constexpr std::uint64_t kMixConstant2 = 0xa0761d6478bd642fULL;

constexpr ProfileParameters kCx18 = {"CX-18", 12ULL << 30, 256ULL << 20, 128ULL << 20, 8, 256, 128, 8192, 24};
constexpr ProfileParameters kCx32 = {"CX-32", 24ULL << 30, 256ULL << 20, 192ULL << 20, 8, 256, 128, 12288, 24};
constexpr ProfileParameters kCx64 = {"CX-64", 48ULL << 30, 512ULL << 20, 256ULL << 20, 8, 256, 128, 16384, 24};
constexpr ProfileParameters kCx128 = {"CX-128", 96ULL << 30, 512ULL << 20, 384ULL << 20, 8, 256, 128, 16384, 24};

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

std::array<std::uint8_t, kEpochCacheBlockBytes> generate_cache_block(
    const ProfileParameters& parameters,
    const Hash256& epoch_seed,
    std::uint64_t block_index) {
  std::vector<std::uint8_t> input;
  constexpr std::size_t domain_size = sizeof(kEpochCacheDomain) - 1;
  input.reserve(domain_size + 1 + 8 + 8 + 4 + 4 + epoch_seed.size() + 8);
  input.insert(input.end(), kEpochCacheDomain, kEpochCacheDomain + domain_size);
  input.push_back(0x00U);
  input.insert(input.end(), parameters.name, parameters.name + 5);
  append_u64_le(parameters.cache_size_bytes, &input);
  append_u64_le(parameters.dataset_size_bytes, &input);
  append_u64_le(parameters.rounds, &input);
  input.insert(input.end(), epoch_seed.begin(), epoch_seed.end());

  const std::vector<std::uint8_t> root = shake256(input, kEpochCacheBlockBytes);
  std::array<std::uint64_t, 8> seed_words{};
  for (std::size_t i = 0; i < seed_words.size(); ++i) {
    seed_words[i] = load_u64_le(&root[i * sizeof(std::uint64_t)]);
  }

  const std::uint64_t profile_bias =
      parameters.cache_size_bytes ^ parameters.dataset_size_bytes ^
      (static_cast<std::uint64_t>(parameters.rounds) << 32U) ^
      static_cast<std::uint64_t>(parameters.get_item_taps);
  std::array<std::uint64_t, 8> words{};
  for (std::size_t i = 0; i < words.size(); ++i) {
    words[i] = seed_words[i] ^ rotate_left(profile_bias + (block_index * kMixConstant0) +
                                               (static_cast<std::uint64_t>(i) * kMixConstant2),
                                           static_cast<unsigned>((9U * i + 17U) & 63U));
  }

  for (std::size_t round = 0; round < 3; ++round) {
    for (std::size_t i = 0; i < words.size(); ++i) {
      const std::uint64_t neighbor0 = words[(i + 1) % words.size()];
      const std::uint64_t neighbor1 = words[(i + 3) % words.size()];
      const std::uint64_t tweak =
          profile_bias + (block_index * kMixConstant0) +
          (static_cast<std::uint64_t>(round) * kMixConstant1) +
          (static_cast<std::uint64_t>(i) * kMixConstant2);
      words[i] ^= rotate_left(neighbor0 + tweak, static_cast<unsigned>((7U * i + 11U * round) & 63U));
      words[i] += neighbor1 ^ rotate_left(profile_bias, static_cast<unsigned>((13U + i + round) & 63U));
      words[i] *= (kMixConstant1 | 1ULL);
      words[i] ^= rotate_left(words[(i + 5) % words.size()], static_cast<unsigned>((19U + 3U * i + round) & 63U));
    }
  }

  std::array<std::uint8_t, kEpochCacheBlockBytes> block{};
  for (std::size_t i = 0; i < words.size(); ++i) {
    store_u64_le(words[i], &block[i * sizeof(std::uint64_t)]);
  }
  return block;
}

void validate_cache_geometry(const ProfileParameters& parameters) {
  if ((parameters.cache_size_bytes % kEpochCacheBlockBytes) != 0U) {
    throw std::invalid_argument("cache size must be a multiple of the cache block size");
  }
}



struct CacheOverrideState {
  bool active = false;
  Profile profile = Profile::kCx18;
  Hash256 epoch_seed{};
  const EpochCache* cache = nullptr;
};

CacheOverrideState& cache_override_state() {
  static thread_local CacheOverrideState state;
  return state;
}

const EpochCache* active_epoch_cache_override(Profile profile, const Hash256& epoch_seed) {
  CacheOverrideState& state = cache_override_state();
  if (!state.active || state.profile != profile || state.epoch_seed != epoch_seed) {
    return nullptr;
  }
  return state.cache;
}

struct CacheMemoEntry {
  bool initialized = false;
  Profile profile = Profile::kCx18;
  Hash256 epoch_seed{};
  EpochCache cache;
};

const EpochCache& memoized_epoch_cache(Profile profile, const Hash256& epoch_seed) {
  if (const EpochCache* override_cache = active_epoch_cache_override(profile, epoch_seed)) {
    return *override_cache;
  }

  static CacheMemoEntry memo;
  if (!memo.initialized || memo.profile != profile || memo.epoch_seed != epoch_seed) {
    memo.profile = profile;
    memo.epoch_seed = epoch_seed;
    rebuild_epoch_cache(profile, epoch_seed, &memo.cache);
    memo.initialized = true;
  }
  return memo.cache;
}

}  // namespace

ScopedEpochCacheOverride::ScopedEpochCacheOverride(Profile profile,
                                                 const Hash256& epoch_seed,
                                                 const EpochCache* cache) {
  if (cache == nullptr) {
    return;
  }
  CacheOverrideState& state = cache_override_state();
  if (state.active) {
    throw std::logic_error("nested epoch cache overrides are not supported");
  }
  state.active = true;
  state.profile = profile;
  state.epoch_seed = epoch_seed;
  state.cache = cache;
  active_ = true;
}

ScopedEpochCacheOverride::~ScopedEpochCacheOverride() {
  if (!active_) {
    return;
  }
  CacheOverrideState& state = cache_override_state();
  state.active = false;
  state.cache = nullptr;
}

const ProfileParameters& profile_parameters(Profile profile) {
  switch (profile) {
    case Profile::kCx18:
      return kCx18;
    case Profile::kCx32:
      return kCx32;
    case Profile::kCx64:
      return kCx64;
    case Profile::kCx128:
      return kCx128;
  }
  throw std::invalid_argument("unknown profile");
}

EpochCache build_epoch_cache(Profile profile, const Hash256& epoch_seed) {
  EpochCache cache;
  rebuild_epoch_cache(profile, epoch_seed, &cache);
  return cache;
}

void rebuild_epoch_cache(Profile profile, const Hash256& epoch_seed, EpochCache* cache) {
  const ProfileParameters& parameters = profile_parameters(profile);
  validate_cache_geometry(parameters);

  cache->assign(static_cast<std::size_t>(parameters.cache_size_bytes), 0U);
  const std::uint64_t block_count = parameters.cache_size_bytes / kEpochCacheBlockBytes;
  for (std::uint64_t block_index = 0; block_index < block_count; ++block_index) {
    const std::array<std::uint8_t, kEpochCacheBlockBytes> block =
        generate_cache_block(parameters, epoch_seed, block_index);
    const std::size_t byte_offset = static_cast<std::size_t>(block_index * kEpochCacheBlockBytes);
    std::copy(block.begin(), block.end(), cache->begin() + byte_offset);
  }
}

std::vector<std::uint8_t> epoch_cache_slice(Profile profile,
                                            const Hash256& epoch_seed,
                                            std::uint64_t offset,
                                            std::size_t length) {
  const ProfileParameters& parameters = profile_parameters(profile);
  validate_cache_geometry(parameters);
  if (offset > parameters.cache_size_bytes ||
      static_cast<std::uint64_t>(length) > (parameters.cache_size_bytes - offset)) {
    throw std::out_of_range("cache slice exceeds cache size");
  }

  const EpochCache& cache = memoized_epoch_cache(profile, epoch_seed);
  std::vector<std::uint8_t> output(length);
  std::copy(cache.begin() + static_cast<std::ptrdiff_t>(offset),
            cache.begin() + static_cast<std::ptrdiff_t>(offset + length),
            output.begin());
  return output;
}

bool epoch_cache_override_active(Profile profile, const Hash256& epoch_seed) {
  return active_epoch_cache_override(profile, epoch_seed) != nullptr;
}

std::uint64_t epoch_cache_word(Profile profile,
                               const Hash256& epoch_seed,
                               std::uint64_t cache_word_index) {
  const ProfileParameters& parameters = profile_parameters(profile);
  validate_cache_geometry(parameters);
  const std::uint64_t byte_offset = cache_word_index * sizeof(std::uint64_t);
  if (byte_offset + sizeof(std::uint64_t) > parameters.cache_size_bytes) {
    throw std::out_of_range("cache word exceeds cache size");
  }
  const EpochCache& cache = memoized_epoch_cache(profile, epoch_seed);
  return load_u64_le(cache.data() + byte_offset);
}

}  // namespace colossusx
