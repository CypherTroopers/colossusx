#include "core/scratch.h"
#include "tests/test_helpers.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
using colossusx::Hash256;
using colossusx::LaneScratch;
using colossusx::Profile;
using colossusx::ScratchUnit;

Hash256 MakeHash(std::uint8_t start) {
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<std::uint8_t>(start + i);
  }
  return hash;
}

std::string UnitHex(const ScratchUnit& unit) {
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (std::uint8_t byte : unit) {
    stream << std::setw(2) << static_cast<unsigned>(byte);
  }
  return stream.str();
}

void TestScratchGeometryMatchesProfiles() {
  EXPECT_EQ(16ULL << 20, colossusx::lane_scratch_bytes(Profile::kCx18));
  EXPECT_EQ((16ULL << 20) / 128ULL, colossusx::lane_scratch_unit_count(Profile::kCx18));
  EXPECT_EQ(24ULL << 20, colossusx::lane_scratch_bytes(Profile::kCx32));
}

void TestScratchPrefixVectorsAcrossLanes() {
  const Hash256 epoch_seed = MakeHash(0x20U);
  const Hash256 header_digest = MakeHash(0x80U);
  EXPECT_EQ(std::string("4cd9cf985f5b587a7153ad3e1824abce98de5dd787c84e2bfaf0f864027e1b492e362d921f2ad3311d632afcd91cb761032ed84ba2ba69ab0e7b8f244bd8ef480a03cc766cada95ee59792b721e92067bf764e20e563a520ad769e8c23edb20b3bef94d6e3eb3f598e45eb9fec9541dc126e7cebbd4ddcc9f5d28de6291c01d0"),
            UnitHex(colossusx::derive_scratch_unit(Profile::kCx18, epoch_seed, header_digest, 42ULL, 0U, 0U)));
  EXPECT_EQ(std::string("caa1818106d5ce089671abccebe0886043997dc05f892489e2fceae4533dfd4d030ae80b883e7df2020c9b5b57c97866f80668ec312018babfad84592b3e8c4ceba370124a079fb575b09ecb2dc84c92060c548992da6b29d5a0e76cb7c164e87a7fa9aea56dddc354b92d4d66bbd5fde5e2a0f8d40f5b539fe1753cd03490a3"),
            UnitHex(colossusx::derive_scratch_unit(Profile::kCx18, epoch_seed, header_digest, 42ULL, 7U, 0U)));
}

void TestWholeScratchHashAndReplaySafeWrites() {
  const Hash256 epoch_seed = MakeHash(0x31U);
  const Hash256 header_digest = MakeHash(0x91U);
  LaneScratch scratch = LaneScratch::Initialize(Profile::kCx18, epoch_seed, header_digest, 99ULL, 3U);
  EXPECT_EQ(std::string("3491b4d4e39dbcb5c5b363331fc374142c2a337eaa9a09795ba6f547aa7137c5"),
            colossusx::to_hex(colossusx::scratch_integrity_hash(scratch)));

  const ScratchUnit original = scratch.ReadUnit(5U);
  ScratchUnit modified = original;
  modified[0] ^= 0xffU;
  modified[127] ^= 0x55U;
  scratch.WriteUnit(5U, modified);
  EXPECT_EQ(UnitHex(modified), UnitHex(scratch.ReadUnit(5U)));
  EXPECT_TRUE(colossusx::to_hex(colossusx::scratch_integrity_hash(scratch)) !=
              std::string("3491b4d4e39dbcb5c5b363331fc374142c2a337eaa9a09795ba6f547aa7137c5"));
}

void TestLaneSpecificInitializationIsDeterministic() {
  const Hash256 epoch_seed = MakeHash(0x44U);
  const Hash256 header_digest = MakeHash(0xa4U);
  const std::string lane_one_hash =
      colossusx::to_hex(colossusx::scratch_integrity_hash(
          LaneScratch::Initialize(Profile::kCx32, epoch_seed, header_digest, 7ULL, 1U)));
  EXPECT_EQ(std::string("d9cdde215896a184195f8ce8dc0cb35e47c1e2ed5e9834b0dd80f1dc53411519"),
            lane_one_hash);
  EXPECT_EQ(lane_one_hash,
            colossusx::to_hex(colossusx::scratch_integrity_hash(
                LaneScratch::Initialize(Profile::kCx32, epoch_seed, header_digest, 7ULL, 1U))));
  EXPECT_TRUE(lane_one_hash !=
              colossusx::to_hex(colossusx::scratch_integrity_hash(
                  LaneScratch::Initialize(Profile::kCx32, epoch_seed, header_digest, 7ULL, 2U))));
}

void RunScratchInitTests() {
  TestScratchGeometryMatchesProfiles();
  TestScratchPrefixVectorsAcrossLanes();
  TestWholeScratchHashAndReplaySafeWrites();
  TestLaneSpecificInitializationIsDeterministic();
}

}  // namespace

void run_scratch_init_tests() { RunScratchInitTests(); }
