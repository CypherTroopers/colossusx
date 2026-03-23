#include "core/epoch_cache.h"
#include "core/epoch_seed.h"
#include "core/pow.h"
#include "core/verifier.h"
#include "tests/test_helpers.h"

#include <cstdint>
#include <vector>

namespace {
using colossusx::BlockHeader;
using colossusx::ChainId;
using colossusx::EpochCache;
using colossusx::Hash256;
using colossusx::Profile;
using colossusx::VerifierInput;
using colossusx::VerifierResult;

Hash256 MakeHash(std::uint8_t start) {
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<std::uint8_t>(start + i);
  }
  return hash;
}

ChainId MakeChainId(std::uint8_t start) {
  ChainId chain_id{};
  for (std::size_t i = 0; i < chain_id.size(); ++i) {
    chain_id[i] = static_cast<std::uint8_t>(start + i);
  }
  return chain_id;
}

BlockHeader MakeHeader() {
  BlockHeader header;
  header.version = 3U;
  header.height = 8643ULL;
  header.timestamp = 1800000123ULL;
  header.bits = 0x1c654657U;
  header.previous_block_hash = MakeHash(0x10U);
  header.transactions_root = MakeHash(0x30U);
  header.state_root = MakeHash(0x50U);
  header.nonce = 999ULL;
  header.pow_result = MakeHash(0x70U);
  return header;
}

Hash256 IncrementTarget(const Hash256& value) {
  Hash256 target = value;
  for (std::size_t i = 0; i < target.size(); ++i) {
    const std::uint16_t next = static_cast<std::uint16_t>(target[i]) + 1U;
    target[i] = static_cast<std::uint8_t>(next & 0xffU);
    if ((next & 0x100U) == 0U) {
      break;
    }
  }
  return target;
}

VerifierInput MakeInput() {
  VerifierInput input;
  input.profile = Profile::kCx18;
  input.header = MakeHeader();
  input.nonce = 0x0123456789abcdefULL;
  input.epoch_seed = colossusx::epoch_seed(MakeChainId(0x21U), 2U, MakeHash(0xa0U));
  input.epoch_cache = colossusx::build_epoch_cache(input.profile, input.epoch_seed);
  input.target.fill(0xffU);
  return input;
}

void TestVerifierReplayMatchesFullReplay() {
  VerifierInput input = MakeInput();
  const Hash256 full_pow_hash =
      colossusx::pow_hash(input.profile, input.epoch_seed, colossusx::header_digest(input.header), input.nonce);

  const VerifierResult result = colossusx::verify_pow(input);
  EXPECT_EQ(colossusx::to_hex(colossusx::header_digest(input.header)), colossusx::to_hex(result.header_digest));
  EXPECT_EQ(colossusx::to_hex(full_pow_hash), colossusx::to_hex(result.pow_hash));
}

void TestVerifierTargetPassingCase() {
  VerifierInput input = MakeInput();
  const Hash256 winning_hash =
      colossusx::pow_hash(input.profile, input.epoch_seed, colossusx::header_digest(input.header), input.nonce);
  input.target = IncrementTarget(winning_hash);

  const VerifierResult result = colossusx::verify_pow(input);
  EXPECT_TRUE(result.target_met);
}

void TestVerifierTargetFailingCases() {
  VerifierInput input = MakeInput();
  const Hash256 winning_hash =
      colossusx::pow_hash(input.profile, input.epoch_seed, colossusx::header_digest(input.header), input.nonce);

  input.target = winning_hash;
  EXPECT_TRUE(!colossusx::verify_pow(input).target_met);

  input.target.fill(0x00U);
  EXPECT_TRUE(!colossusx::verify_pow(input).target_met);
}

void TestVerifierUsesProvidedEpochCacheWithoutResidentDataset() {
  VerifierInput input = MakeInput();
  const Hash256 expected =
      colossusx::pow_hash(input.profile, input.epoch_seed, colossusx::header_digest(input.header), input.nonce);

  EpochCache corrupted_cache = input.epoch_cache;
  corrupted_cache[17] ^= 0x5aU;
  corrupted_cache[corrupted_cache.size() - 9U] ^= 0xa5U;
  input.epoch_cache = corrupted_cache;

  const VerifierResult corrupted = colossusx::verify_pow(input);
  EXPECT_TRUE(colossusx::to_hex(corrupted.pow_hash) != colossusx::to_hex(expected));
}

void RunVerifierTests() {
  TestVerifierReplayMatchesFullReplay();
  TestVerifierTargetPassingCase();
  TestVerifierTargetFailingCases();
  TestVerifierUsesProvidedEpochCacheWithoutResidentDataset();
}

}  // namespace

void run_verifier_tests() { RunVerifierTests(); }
