#include "core/shake256.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace colossusx {
namespace {

constexpr std::size_t kKeccakStateWords = 25;
constexpr std::size_t kShake256RateBytes = 136;
constexpr std::uint8_t kShakeDomainSuffix = 0x1fU;

constexpr std::array<std::uint64_t, 24> kRoundConstants = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL};

constexpr std::array<unsigned, kKeccakStateWords> kRotation = {
    0,  1, 62, 28, 27, 36, 44, 6,  55, 20, 3, 10, 43,
    25, 39, 41, 45, 15, 21, 8,  18, 2, 61, 56, 14};

constexpr std::array<unsigned, kKeccakStateWords> kPiLane = {
    0,  10, 20, 5, 15, 16, 1,  11, 21, 6,  7, 17, 2,
    12, 22, 23, 8,  18, 3, 13, 14, 24, 9,  19, 4};

std::uint64_t rotate_left(std::uint64_t value, unsigned shift) {
  return shift == 0 ? value : ((value << shift) | (value >> (64U - shift)));
}

std::uint64_t load_u64_le(const std::uint8_t* input) {
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < 8; ++i) {
    value |= static_cast<std::uint64_t>(input[i]) << (8U * i);
  }
  return value;
}

void store_u64_le(std::uint64_t value, std::uint8_t* output) {
  for (std::size_t i = 0; i < 8; ++i) {
    output[i] = static_cast<std::uint8_t>((value >> (8U * i)) & 0xffU);
  }
}

void keccak_f1600(std::array<std::uint64_t, kKeccakStateWords>* state) {
  for (std::uint64_t round_constant : kRoundConstants) {
    std::array<std::uint64_t, 5> c{};
    std::array<std::uint64_t, 5> d{};
    for (std::size_t x = 0; x < 5; ++x) {
      c[x] = (*state)[x] ^ (*state)[x + 5] ^ (*state)[x + 10] ^ (*state)[x + 15] ^ (*state)[x + 20];
    }
    for (std::size_t x = 0; x < 5; ++x) {
      d[x] = c[(x + 4) % 5] ^ rotate_left(c[(x + 1) % 5], 1);
    }
    for (std::size_t y = 0; y < 5; ++y) {
      for (std::size_t x = 0; x < 5; ++x) {
        (*state)[x + 5 * y] ^= d[x];
      }
    }

    std::array<std::uint64_t, kKeccakStateWords> b{};
    for (std::size_t i = 0; i < kKeccakStateWords; ++i) {
      b[kPiLane[i]] = rotate_left((*state)[i], kRotation[i]);
    }

    for (std::size_t y = 0; y < 5; ++y) {
      for (std::size_t x = 0; x < 5; ++x) {
        (*state)[x + 5 * y] = b[x + 5 * y] ^ ((~b[((x + 1) % 5) + 5 * y]) & b[((x + 2) % 5) + 5 * y]);
      }
    }

    (*state)[0] ^= round_constant;
  }
}

}  // namespace

std::vector<std::uint8_t> shake256(const std::vector<std::uint8_t>& input,
                                   std::size_t output_size) {
  std::array<std::uint64_t, kKeccakStateWords> state{};
  std::size_t offset = 0;
  while (offset + kShake256RateBytes <= input.size()) {
    for (std::size_t i = 0; i < kShake256RateBytes / 8; ++i) {
      state[i] ^= load_u64_le(&input[offset + i * 8]);
    }
    keccak_f1600(&state);
    offset += kShake256RateBytes;
  }

  std::array<std::uint8_t, kShake256RateBytes> block{};
  const std::size_t remaining = input.size() - offset;
  for (std::size_t i = 0; i < remaining; ++i) {
    block[i] = input[offset + i];
  }
  block[remaining] ^= kShakeDomainSuffix;
  block[kShake256RateBytes - 1] ^= 0x80U;
  for (std::size_t i = 0; i < kShake256RateBytes / 8; ++i) {
    state[i] ^= load_u64_le(&block[i * 8]);
  }
  keccak_f1600(&state);

  std::vector<std::uint8_t> output(output_size);
  std::size_t produced = 0;
  while (produced < output_size) {
    const std::size_t chunk = std::min(kShake256RateBytes, output_size - produced);
    for (std::size_t i = 0; i < chunk / 8; ++i) {
      store_u64_le(state[i], &output[produced + i * 8]);
    }
    if ((chunk % 8) != 0) {
      std::array<std::uint8_t, 8> tail{};
      store_u64_le(state[chunk / 8], tail.data());
      for (std::size_t i = 0; i < (chunk % 8); ++i) {
        output[produced + (chunk / 8) * 8 + i] = tail[i];
      }
    }
    produced += chunk;
    if (produced < output_size) {
      keccak_f1600(&state);
    }
  }
  return output;
}

}  // namespace colossusx
