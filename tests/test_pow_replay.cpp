#include "core/pow.h"
#include "core/shake256.h"
#include "tests/test_helpers.h"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {
using colossusx::FoldedReplayState;
using colossusx::Hash256;
using colossusx::LaneReplayResult;
using colossusx::Profile;

Hash256 MakeHash(std::uint8_t start) {
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<std::uint8_t>(start + i);
  }
  return hash;
}

std::string StateHex(const std::array<std::uint64_t, 4>& values) {
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (std::uint64_t value : values) {
    stream << std::setw(16) << value;
  }
  return stream.str();
}

std::string LaneFieldHashHex(const std::vector<LaneReplayResult>& results, bool state_x) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(results.size() * 4U * sizeof(std::uint64_t));
  for (const LaneReplayResult& result : results) {
    const auto& words = state_x ? result.final_state.x : result.final_state.a;
    for (std::uint64_t word : words) {
      for (std::size_t i = 0; i < sizeof(word); ++i) {
        bytes.push_back(static_cast<std::uint8_t>((word >> (8U * i)) & 0xffU));
      }
    }
  }
  return colossusx::to_hex(colossusx::shake256(bytes, colossusx::kHashBytes));
}

void TestFullReplayCx18DeterministicVector() {
  const Hash256 epoch_seed = MakeHash(0x52U);
  const Hash256 header_digest = MakeHash(0x90U);
  const std::vector<LaneReplayResult> lanes =
      colossusx::replay_nonce(Profile::kCx18, epoch_seed, header_digest, 0x0123456789abcdefULL);
  EXPECT_EQ(8U, lanes.size());
  EXPECT_EQ(0U, lanes.front().lane_id);
  EXPECT_EQ(7U, lanes.back().lane_id);
  EXPECT_EQ(std::string("0129b9a03f1a00c7ed1655d8db24f05de480a8cb31ffee3fd3ca4b71e64b5240"),
            LaneFieldHashHex(lanes, true));
  EXPECT_EQ(std::string("90b57e93cc60e30a3581c138810b48bf0116fb8adca99d12b4c1ee3152b0df4d"),
            LaneFieldHashHex(lanes, false));

  const FoldedReplayState folded = colossusx::fold_lane_results(Profile::kCx18, lanes);
  EXPECT_EQ(std::string("43cd88075046aec4627dcaec595d7f9ed9c04fdfb505af66c0710af0e28c6b24"),
            StateHex(folded.x));
  EXPECT_EQ(std::string("c40aa2076f307ac0b2b8a1d2c58222e5833bc85119034ea83757c3e14d9889fd"),
            StateHex(folded.a));
  EXPECT_EQ(std::string("6f8352a921e9ae8ef369296a7353424fae9dc40b2134cc5a63e94de1735aa1bd"),
            colossusx::to_hex(colossusx::compute_pow_hash(Profile::kCx18, folded)));

}

void TestNonceChangeChangesPowHash() {
  const Hash256 epoch_seed = MakeHash(0x52U);
  const Hash256 header_digest = MakeHash(0x90U);
  const Hash256 hash_a = colossusx::pow_hash(Profile::kCx18, epoch_seed, header_digest, 11ULL);
  const Hash256 hash_b = colossusx::pow_hash(Profile::kCx18, epoch_seed, header_digest, 12ULL);
  EXPECT_TRUE(hash_a != hash_b);
}

void RunPowReplayTests() {
  TestFullReplayCx18DeterministicVector();
  TestNonceChangeChangesPowHash();
}

}  // namespace

void run_pow_replay_tests() { RunPowReplayTests(); }
