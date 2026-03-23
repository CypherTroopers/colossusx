#ifndef COLOSSUSX_BENCH_BENCH_INPUTS_H_
#define COLOSSUSX_BENCH_BENCH_INPUTS_H_

#include "core/epoch_seed.h"

namespace colossusx {
namespace bench {

inline Hash256 benchmark_hash(std::uint8_t start) {
  Hash256 hash{};
  for (std::size_t i = 0; i < hash.size(); ++i) {
    hash[i] = static_cast<std::uint8_t>(start + i);
  }
  return hash;
}

inline ChainId benchmark_chain_id(std::uint8_t start) {
  ChainId chain_id{};
  for (std::size_t i = 0; i < chain_id.size(); ++i) {
    chain_id[i] = static_cast<std::uint8_t>(start + i);
  }
  return chain_id;
}

inline BlockHeader benchmark_header() {
  BlockHeader header;
  header.version = 3U;
  header.height = 8643ULL;
  header.timestamp = 1800000123ULL;
  header.bits = 0x1c654657U;
  header.previous_block_hash = benchmark_hash(0x10U);
  header.transactions_root = benchmark_hash(0x30U);
  header.state_root = benchmark_hash(0x50U);
  header.nonce = 0U;
  header.pow_result = benchmark_hash(0x70U);
  return header;
}

inline Hash256 benchmark_epoch_seed() {
  return epoch_seed(benchmark_chain_id(0x21U), 2U, benchmark_hash(0xa0U));
}

inline Hash256 benchmark_header_digest() {
  return header_digest(benchmark_header());
}

inline std::uint64_t benchmark_nonce() {
  return 0x0123456789abcdefULL;
}

}  // namespace bench
}  // namespace colossusx

#endif
