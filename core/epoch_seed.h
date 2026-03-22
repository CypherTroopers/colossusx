#ifndef COLOSSUSX_CORE_EPOCH_SEED_H_
#define COLOSSUSX_CORE_EPOCH_SEED_H_

#include <cstdint>

#include "core/header.h"

namespace colossusx {

Hash256 epoch_seed(const ChainId& chain_id,
                   std::uint64_t epoch_index,
                   const Hash256& prev_epoch_anchor_hash);

}  // namespace colossusx

#endif
