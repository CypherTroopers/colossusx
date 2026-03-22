#include "core/round_function.h"
#include "tests/test_helpers.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
using colossusx::Hash256;
using colossusx::LaneScratch;
using colossusx::LaneState;
using colossusx::Profile;
using colossusx::RoundTrace;
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

std::string StateHex(const std::array<std::uint64_t, 4>& values) {
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (std::uint64_t value : values) {
    stream << std::setw(16) << value;
  }
  return stream.str();
}

LaneState MakeLaneState() {
  LaneState state;
  state.x = {0x0123456789abcdefULL, 0x1111222233334444ULL,
             0x5555666677778888ULL, 0x9999aaaabbbbccccULL};
  state.a = {0x0fedcba987654321ULL, 0x2222333344445555ULL,
             0x6666777788889999ULL, 0xaaaabbbbccccddddULL};
  return state;
}

void TestOneLaneOneRoundTrace() {
  const Hash256 epoch_seed = MakeHash(0x10U);
  const Hash256 header_digest = MakeHash(0x40U);
  LaneScratch scratch = LaneScratch::Initialize(Profile::kCx18, epoch_seed, header_digest, 5ULL, 0U);
  const LaneState initial = MakeLaneState();
  const RoundTrace trace = colossusx::execute_round(Profile::kCx18, epoch_seed, 0U, initial, &scratch);

  EXPECT_EQ(23912ULL, trace.scratch_read_index0);
  EXPECT_EQ(19133ULL, trace.scratch_read_index1);
  EXPECT_EQ(12517659ULL, trace.item_index0);
  EXPECT_EQ(3910353ULL, trace.item_index1);
  EXPECT_EQ(20318152ULL, trace.item_index2);
  EXPECT_EQ(60578ULL, trace.scratch_write_index);
  EXPECT_EQ(std::string("aa6bf069d06e8315862b491e3dcbced206611329d87927b97da70f627de89ed6"),
            StateHex(trace.next_state.x));
  EXPECT_EQ(std::string("a2d0e1a256c12eea1ac75ffbc876a157e4812f167595237083e710bc3b37f61b"),
            StateHex(trace.next_state.a));
}

void TestRoundWritesExpectedScratchUnitPrefix() {
  const Hash256 epoch_seed = MakeHash(0x10U);
  const Hash256 header_digest = MakeHash(0x40U);
  LaneScratch scratch = LaneScratch::Initialize(Profile::kCx18, epoch_seed, header_digest, 5ULL, 0U);
  const RoundTrace trace = colossusx::execute_round(Profile::kCx18, epoch_seed, 0U, MakeLaneState(), &scratch);
  EXPECT_EQ(std::string("088f83361d3b2aaee3f374be5651817d4c4269ac91da84615d0fbffc2029af15300eadb2b4afb92c3c445c96be8ea9a72c1f215a4d55695ce607885569a4dd32ca0e64ddacda2386e9f333445ea7996983b79b2fdef327a7615fe45fecb02928acbed64a720da1c92744a3e35fdec7666c17df2459f64038af941dd7071339f1"),
            UnitHex(trace.scratch_write_unit));
  EXPECT_EQ(UnitHex(trace.scratch_write_unit),
            UnitHex(scratch.ReadUnit(trace.scratch_write_index)));
}

void TestDependencyChainIsReal() {
  const Hash256 epoch_seed = MakeHash(0x10U);
  const Hash256 header_digest = MakeHash(0x40U);
  const LaneState initial = MakeLaneState();

  LaneScratch baseline_scratch = LaneScratch::Initialize(Profile::kCx18, epoch_seed, header_digest, 5ULL, 0U);
  const RoundTrace baseline = colossusx::execute_round(Profile::kCx18, epoch_seed, 0U, initial, &baseline_scratch);

  LaneScratch altered_scratch = LaneScratch::Initialize(Profile::kCx18, epoch_seed, header_digest, 5ULL, 0U);
  ScratchUnit modified = altered_scratch.ReadUnit(baseline.scratch_read_index0);
  modified[40] ^= 0x5aU;
  altered_scratch.WriteUnit(baseline.scratch_read_index0, modified);
  const RoundTrace altered = colossusx::execute_round(Profile::kCx18, epoch_seed, 0U, initial, &altered_scratch);

  EXPECT_EQ(baseline.item_index0, altered.item_index0);
  EXPECT_TRUE(baseline.item_index1 != altered.item_index1);
  EXPECT_TRUE(baseline.item_index2 != altered.item_index2);
  EXPECT_TRUE(StateHex(baseline.next_state.x) != StateHex(altered.next_state.x));
}

void RunRoundFunctionTests() {
  TestOneLaneOneRoundTrace();
  TestRoundWritesExpectedScratchUnitPrefix();
  TestDependencyChainIsReal();
}

}  // namespace

void run_round_function_tests() { RunRoundFunctionTests(); }
