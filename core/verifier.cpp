#include "core/verifier.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "core/pow.h"

namespace colossusx {
namespace {

void validate_epoch_cache(Profile profile, const EpochCache& cache) {
  const ProfileParameters& parameters = profile_parameters(profile);
  if (cache.size() != parameters.cache_size_bytes) {
    throw std::invalid_argument("verifier epoch cache size does not match profile cache size");
  }
}

}  // namespace

VerifierResult verify_pow(const VerifierInput& input) {
  validate_epoch_cache(input.profile, input.epoch_cache);

  VerifierResult result;
  result.header_digest = header_digest(input.header);

  const ScopedEpochCacheOverride cache_scope(input.profile, input.epoch_seed, &input.epoch_cache);
  result.pow_hash = pow_hash(input.profile, input.epoch_seed, result.header_digest, input.nonce);
  result.target_met = pow_hash_meets_target(result.pow_hash, input.target);
  return result;
}

bool pow_hash_meets_target(const Hash256& pow_hash, const Hash256& target) {
  for (std::size_t i = kHashBytes; i > 0; --i) {
    const std::size_t index = i - 1U;
    if (pow_hash[index] < target[index]) {
      return true;
    }
    if (pow_hash[index] > target[index]) {
      return false;
    }
  }
  return false;
}

}  // namespace colossusx
