#include "core/epoch_cache.h"
#include "core/shake256.h"
#include "tests/test_helpers.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
using colossusx::Hash256;
using colossusx::Profile;
using colossusx::ProfileParameters;

Hash256 MakeSeed(std::uint8_t start) {
  Hash256 seed{};
  for (std::size_t i = 0; i < seed.size(); ++i) {
    seed[i] = static_cast<std::uint8_t>(start + i);
  }
  return seed;
}

std::string HexSlice(Profile profile, const Hash256& seed, std::uint64_t offset, std::size_t length) {
  return colossusx::to_hex(colossusx::epoch_cache_slice(profile, seed, offset, length));
}

void TestProfileTableMatchesReference() {
  const ProfileParameters& cx18 = colossusx::profile_parameters(Profile::kCx18);
  EXPECT_EQ(std::string("CX-18"), std::string(cx18.name));
  EXPECT_EQ(12ULL << 30, cx18.dataset_size_bytes);
  EXPECT_EQ(256ULL << 20, cx18.cache_size_bytes);
  EXPECT_EQ(128ULL << 20, cx18.scratch_total_bytes);
  EXPECT_EQ(8192U, cx18.rounds);
  EXPECT_EQ(24U, cx18.get_item_taps);

  const ProfileParameters& cx64 = colossusx::profile_parameters(Profile::kCx64);
  EXPECT_EQ(std::string("CX-64"), std::string(cx64.name));
  EXPECT_EQ(512ULL << 20, cx64.cache_size_bytes);
  EXPECT_EQ(16384U, cx64.rounds);
}

void TestCacheSliceVectorsForCx18AndCx32() {
  const Hash256 seed = MakeSeed(0x20U);
  EXPECT_EQ(std::string("2878c4b05e939245c83e327c49ad2235162530711bfb40f7aaa1d7d41fd5714b"),
            HexSlice(Profile::kCx18, seed, 0, 32));
  EXPECT_EQ(std::string("b0d34a7723fbab6e6819730b209734c91cf1f24fb147cfcfeeb105c520e856e8"),
            HexSlice(Profile::kCx18, seed, 4096, 32));

  const std::uint64_t cx18_tail =
      colossusx::profile_parameters(Profile::kCx18).cache_size_bytes - 32ULL;
  EXPECT_EQ(std::string("064b139df80d7a4dd2c2665efd8098faeb77c1a328f100e0f78d12d2627554c5"),
            HexSlice(Profile::kCx18, seed, cx18_tail, 32));

  EXPECT_EQ(std::string("0b94cb1201157e96f5d19492ad6636b4939b0eb638b302559bb1eaad5bb74633"),
            HexSlice(Profile::kCx32, seed, 0, 32));
  EXPECT_EQ(std::string("f95a5e20b3dc7e3738b57b95c4a49b3adc8e817953ee7f51085d13ca32a4209e"),
            HexSlice(Profile::kCx32, seed, 8192, 32));
}

void TestCacheSliceRebuildIsConsistentAcrossBlockBoundaries() {
  const Hash256 seed = MakeSeed(0x40U);
  const std::vector<std::uint8_t> wide = colossusx::epoch_cache_slice(Profile::kCx18, seed, 48, 96);
  const std::vector<std::uint8_t> left = colossusx::epoch_cache_slice(Profile::kCx18, seed, 48, 48);
  const std::vector<std::uint8_t> right = colossusx::epoch_cache_slice(Profile::kCx18, seed, 96, 48);

  for (std::size_t i = 0; i < left.size(); ++i) {
    EXPECT_EQ(static_cast<unsigned>(wide[i]), static_cast<unsigned>(left[i]));
  }
  for (std::size_t i = 0; i < right.size(); ++i) {
    EXPECT_EQ(static_cast<unsigned>(wide[left.size() + i]), static_cast<unsigned>(right[i]));
  }
}

void TestCacheGenerationIsDeterministicAndProfileSeparated() {
  const Hash256 seed = MakeSeed(0x60U);
  const std::string cx18_sample = HexSlice(Profile::kCx18, seed, 12345, 48);
  EXPECT_EQ(cx18_sample, HexSlice(Profile::kCx18, seed, 12345, 48));
  EXPECT_TRUE(cx18_sample != HexSlice(Profile::kCx18, MakeSeed(0x61U), 12345, 48));
  EXPECT_TRUE(cx18_sample != HexSlice(Profile::kCx32, seed, 12345, 48));
}

void TestVisibleSampleDigestForFutureTv003() {
  const Hash256 seed = MakeSeed(0x33U);
  std::vector<std::uint8_t> samples;
  const std::array<std::uint64_t, 3> offsets = {0ULL, 4096ULL, (1ULL << 20) + 128ULL};
  for (std::uint64_t offset : offsets) {
    const std::vector<std::uint8_t> sample = colossusx::epoch_cache_slice(Profile::kCx18, seed, offset, 32);
    samples.insert(samples.end(), sample.begin(), sample.end());
  }
  const std::string digest = colossusx::to_hex(colossusx::shake256(samples, 32));
  EXPECT_EQ(std::string("f5823b71088cce713ddda5c8ed034ebef6e8856ef91ba197e76fda664abe0110"),
            digest);
}

void RunEpochCacheTests() {
  TestProfileTableMatchesReference();
  TestCacheSliceVectorsForCx18AndCx32();
  TestCacheSliceRebuildIsConsistentAcrossBlockBoundaries();
  TestCacheGenerationIsDeterministicAndProfileSeparated();
  TestVisibleSampleDigestForFutureTv003();
}

}  // namespace

void run_epoch_cache_tests() { RunEpochCacheTests(); }
