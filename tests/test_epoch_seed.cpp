#include "core/epoch_seed.h"
#include "core/header.h"
#include "tests/test_helpers.h"

#include <iostream>

namespace {
using colossusx::ChainId;
using colossusx::Hash256;

ChainId MakeChainId(std::uint8_t start) {
  ChainId chain_id{};
  for (std::size_t i = 0; i < chain_id.size(); ++i) {
    chain_id[i] = static_cast<std::uint8_t>(start + i);
  }
  return chain_id;
}

Hash256 MakeHash(std::uint8_t start) {
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<std::uint8_t>(start + i);
  }
  return hash;
}

void TestEpochIndexDerivation() {
  EXPECT_EQ(static_cast<std::uint64_t>(0), colossusx::epoch_index_from_height(0));
  EXPECT_EQ(static_cast<std::uint64_t>(0), colossusx::epoch_index_from_height(4319));
  EXPECT_EQ(static_cast<std::uint64_t>(1), colossusx::epoch_index_from_height(4320));
  EXPECT_EQ(static_cast<std::uint64_t>(2), colossusx::epoch_index_from_height(8640));
  EXPECT_EQ(static_cast<std::uint64_t>(1024), colossusx::epoch_index_from_height(1024ULL * 4320ULL));
}

void TestEpochSeedIsStable() {
  const ChainId chain_id = MakeChainId(0x10U);
  const Hash256 anchor_hash = MakeHash(0x80U);
  const Hash256 seed = colossusx::epoch_seed(chain_id, 17, anchor_hash);
  EXPECT_EQ(std::string("5d8bb4d804a7803a343052066057914d2a95109b30d9958c24bfee9b3341242e"),
            colossusx::to_hex(seed));
}

void TestEpochSeedDomainSeparatesInputs() {
  const ChainId chain_id = MakeChainId(0x10U);
  const Hash256 anchor_hash = MakeHash(0x80U);
  const std::string base = colossusx::to_hex(colossusx::epoch_seed(chain_id, 17, anchor_hash));
  EXPECT_TRUE(base != colossusx::to_hex(colossusx::epoch_seed(chain_id, 18, anchor_hash)));
  EXPECT_TRUE(base != colossusx::to_hex(colossusx::epoch_seed(MakeChainId(0x11U), 17, anchor_hash)));
  EXPECT_TRUE(base != colossusx::to_hex(colossusx::epoch_seed(chain_id, 17, MakeHash(0x81U))));
}

void RunEpochTests() {
  TestEpochIndexDerivation();
  TestEpochSeedIsStable();
  TestEpochSeedDomainSeparatesInputs();
}

}  // namespace

void run_epoch_seed_tests() { RunEpochTests(); }
