#include "core/header.h"
#include "tests/test_helpers.h"

#include <iostream>

namespace {
using colossusx::BlockHeader;
using colossusx::Hash256;

Hash256 MakeSequentialHash(std::uint8_t start) {
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<std::uint8_t>(start + i);
  }
  return hash;
}

void TestCanonicalSerializationExcludesNonceAndPow() {
  BlockHeader header;
  header.version = 0x01020304U;
  header.height = 0x1122334455667788ULL;
  header.timestamp = 0x8877665544332211ULL;
  header.bits = 0xa1b2c3d4U;
  header.previous_block_hash = MakeSequentialHash(0x10U);
  header.transactions_root = MakeSequentialHash(0x40U);
  header.state_root = MakeSequentialHash(0x70U);
  header.nonce = 0xffffffffffffffffULL;
  header.pow_result = MakeSequentialHash(0xa0U);

  const std::string expected =
      "04030201"
      "8877665544332211"
      "1122334455667788"
      "d4c3b2a1"
      "101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f"
      "404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f"
      "707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f";
  EXPECT_EQ(expected, colossusx::to_hex(colossusx::canonical_header_serialization(header)));
}

void TestHeaderDigestIsStable() {
  BlockHeader header;
  header.version = 1;
  header.height = 4321;
  header.timestamp = 1717171717ULL;
  header.bits = 0x1d00ffffU;
  header.previous_block_hash = MakeSequentialHash(0x01U);
  header.transactions_root = MakeSequentialHash(0x21U);
  header.state_root = MakeSequentialHash(0x41U);
  header.nonce = 7;
  header.pow_result = MakeSequentialHash(0x61U);

  const std::string digest = colossusx::to_hex(colossusx::header_digest(header));
  EXPECT_EQ(digest, colossusx::to_hex(colossusx::header_digest(header)));
  EXPECT_EQ(std::string("0380243fc030d395f48f400bce15590bc3a43c00faafdffc3c8b07489d843dcb"), digest);
}

void TestHeaderDigestIgnoresNonceAndPowResult() {
  BlockHeader header_a;
  header_a.version = 2;
  header_a.height = 8640;
  header_a.timestamp = 1800000000ULL;
  header_a.bits = 0x1c654657U;
  header_a.previous_block_hash = MakeSequentialHash(0x05U);
  header_a.transactions_root = MakeSequentialHash(0x25U);
  header_a.state_root = MakeSequentialHash(0x45U);
  header_a.nonce = 1;
  header_a.pow_result = MakeSequentialHash(0x65U);

  BlockHeader header_b = header_a;
  header_b.nonce = 999999U;
  header_b.pow_result = MakeSequentialHash(0x95U);

  EXPECT_EQ(colossusx::to_hex(colossusx::header_digest(header_a)),
            colossusx::to_hex(colossusx::header_digest(header_b)));
}

void RunHeaderTests() {
  TestCanonicalSerializationExcludesNonceAndPow();
  TestHeaderDigestIsStable();
  TestHeaderDigestIgnoresNonceAndPowResult();
}

}  // namespace

void run_header_digest_tests() { RunHeaderTests(); }
