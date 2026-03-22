#ifndef COLOSSUSX_CORE_ROUND_FUNCTION_H_
#define COLOSSUSX_CORE_ROUND_FUNCTION_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/get_item.h"
#include "core/scratch.h"

namespace colossusx {

struct LaneState {
  std::array<std::uint64_t, 4> x{};
  std::array<std::uint64_t, 4> a{};
};

struct RoundTrace {
  std::uint64_t scratch_read_index0 = 0;
  std::uint64_t scratch_read_index1 = 0;
  std::uint64_t scratch_write_index = 0;
  std::uint64_t item_index0 = 0;
  std::uint64_t item_index1 = 0;
  std::uint64_t item_index2 = 0;
  std::uint64_t t0 = 0;
  std::uint64_t t1 = 0;
  std::uint64_t t2 = 0;
  ScratchUnit scratch_write_unit{};
  LaneState next_state{};
};

RoundTrace execute_round(Profile profile,
                         const Hash256& epoch_seed,
                         std::uint64_t round_index,
                         const LaneState& lane_state,
                         LaneScratch* scratch);

}  // namespace colossusx

#endif
