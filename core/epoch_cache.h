#ifndef COLOSSUSX_CORE_EPOCH_CACHE_H_
#define COLOSSUSX_CORE_EPOCH_CACHE_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/header.h"

namespace colossusx {

enum class Profile {
  kCx18,
  kCx32,
  kCx64,
  kCx128,
};

struct ProfileParameters {
  const char* name;
  std::uint64_t dataset_size_bytes;
  std::uint64_t cache_size_bytes;
  std::uint64_t scratch_total_bytes;
  std::uint32_t lanes;
  std::uint32_t item_size_bytes;
  std::uint32_t scratch_unit_bytes;
  std::uint32_t rounds;
  std::uint32_t get_item_taps;
};

const ProfileParameters& profile_parameters(Profile profile);

using EpochCache = std::vector<std::uint8_t>;

class ScopedEpochCacheOverride {
 public:
  ScopedEpochCacheOverride(Profile profile,
                           const Hash256& epoch_seed,
                           const EpochCache* cache);
  ~ScopedEpochCacheOverride();

  ScopedEpochCacheOverride(const ScopedEpochCacheOverride&) = delete;
  ScopedEpochCacheOverride& operator=(const ScopedEpochCacheOverride&) = delete;

 private:
  bool active_ = false;
};

EpochCache build_epoch_cache(Profile profile, const Hash256& epoch_seed);
void rebuild_epoch_cache(Profile profile, const Hash256& epoch_seed, EpochCache* cache);
std::vector<std::uint8_t> epoch_cache_slice(Profile profile,
                                            const Hash256& epoch_seed,
                                            std::uint64_t offset,
                                            std::size_t length);
std::uint64_t epoch_cache_word(Profile profile,
                               const Hash256& epoch_seed,
                               std::uint64_t cache_word_index);
bool epoch_cache_override_active(Profile profile, const Hash256& epoch_seed);

}  // namespace colossusx

#endif
