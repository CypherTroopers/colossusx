#ifndef COLOSSUSX_CORE_POW_H_
#define COLOSSUSX_CORE_POW_H_

#include <array>
#include <cstdint>
#include <vector>

#include "core/round_function.h"

namespace colossusx {

struct LaneReplayResult {
  std::uint32_t lane_id = 0;
  LaneState final_state{};
};

struct FoldedReplayState {
  std::array<std::uint64_t, 4> x{};
  std::array<std::uint64_t, 4> a{};
};

LaneState initialize_lane_state(Profile profile,
                                const Hash256& epoch_seed,
                                const Hash256& header_digest,
                                std::uint64_t nonce,
                                std::uint32_t lane_id);

LaneReplayResult replay_lane(Profile profile,
                             const Hash256& epoch_seed,
                             const Hash256& header_digest,
                             std::uint64_t nonce,
                             std::uint32_t lane_id);

std::vector<LaneReplayResult> replay_nonce(Profile profile,
                                           const Hash256& epoch_seed,
                                           const Hash256& header_digest,
                                           std::uint64_t nonce);

FoldedReplayState fold_lane_results(Profile profile,
                                    const std::vector<LaneReplayResult>& lane_results);

Hash256 compute_pow_hash(Profile profile,
                         const FoldedReplayState& folded_state);

Hash256 pow_hash(Profile profile,
                 const Hash256& epoch_seed,
                 const Hash256& header_digest,
                 std::uint64_t nonce);

}  // namespace colossusx

#endif
