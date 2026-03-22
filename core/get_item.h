#ifndef COLOSSUSX_CORE_GET_ITEM_H_
#define COLOSSUSX_CORE_GET_ITEM_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/epoch_cache.h"

namespace colossusx {

constexpr std::size_t kDatasetItemBytes = 256;
using DatasetItem = std::array<std::uint8_t, kDatasetItemBytes>;

std::uint64_t dataset_item_count(Profile profile);
DatasetItem get_item(Profile profile, const Hash256& epoch_seed, std::uint64_t item_index);

}  // namespace colossusx

#endif
