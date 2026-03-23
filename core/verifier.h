#ifndef COLOSSUSX_CORE_VERIFIER_H_
#define COLOSSUSX_CORE_VERIFIER_H_

#include "core/epoch_cache.h"
#include "core/header.h"

namespace colossusx {

struct VerifierInput {
  Profile profile = Profile::kCx18;
  BlockHeader header{};
  std::uint64_t nonce = 0;
  Hash256 target{};
  Hash256 epoch_seed{};
  EpochCache epoch_cache;
};

struct VerifierResult {
  Hash256 header_digest{};
  Hash256 pow_hash{};
  bool target_met = false;
};

VerifierResult verify_pow(const VerifierInput& input);
bool pow_hash_meets_target(const Hash256& pow_hash, const Hash256& target);

}  // namespace colossusx

#endif
