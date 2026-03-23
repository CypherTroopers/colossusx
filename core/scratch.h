#ifndef COLOSSUSX_CORE_SCRATCH_H_
#define COLOSSUSX_CORE_SCRATCH_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "core/epoch_cache.h"

namespace colossusx {

constexpr std::size_t kScratchUnitBytes = 128;
using ScratchUnit = std::array<std::uint8_t, kScratchUnitBytes>;

std::uint64_t lane_scratch_bytes(Profile profile);
std::uint64_t lane_scratch_unit_count(Profile profile);

ScratchUnit derive_scratch_unit(Profile profile,
                                const Hash256& epoch_seed,
                                const Hash256& header_digest,
                                std::uint64_t nonce,
                                std::uint32_t lane_id,
                                std::uint64_t unit_index);

class LaneScratch {
 public:
  static LaneScratch Initialize(Profile profile,
                                const Hash256& epoch_seed,
                                const Hash256& header_digest,
                                std::uint64_t nonce,
                                std::uint32_t lane_id);
  static LaneScratch InitializeLazy(Profile profile,
                                    const Hash256& epoch_seed,
                                    const Hash256& header_digest,
                                    std::uint64_t nonce,
                                    std::uint32_t lane_id);

  Profile profile() const { return profile_; }
  std::uint32_t lane_id() const { return lane_id_; }
  std::size_t size_bytes() const;
  std::uint64_t unit_count() const;

  ScratchUnit ReadUnit(std::uint64_t unit_index) const;
  void WriteUnit(std::uint64_t unit_index, const ScratchUnit& unit);
  const std::vector<std::uint8_t>& bytes() const { return bytes_; }

 private:
  LaneScratch(Profile profile, std::uint32_t lane_id, std::vector<std::uint8_t> bytes);
  LaneScratch(Profile profile,
              std::uint32_t lane_id,
              std::array<std::uint64_t, kScratchUnitBytes / sizeof(std::uint64_t)> root_words,
              std::uint64_t nonce);

  Profile profile_;
  std::uint32_t lane_id_;
  std::vector<std::uint8_t> bytes_;
  bool lazy_ = false;
  std::array<std::uint64_t, kScratchUnitBytes / sizeof(std::uint64_t)> root_words_{};
  std::uint64_t lazy_nonce_ = 0;
  mutable std::unordered_map<std::uint64_t, ScratchUnit> materialized_units_;
};

Hash256 scratch_integrity_hash(const LaneScratch& scratch);

}  // namespace colossusx

#endif
