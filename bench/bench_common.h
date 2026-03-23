#ifndef COLOSSUSX_BENCH_BENCH_COMMON_H_
#define COLOSSUSX_BENCH_BENCH_COMMON_H_

#include <cstdint>
#include <string>
#include <vector>

#include "core/epoch_cache.h"
#include "core/header.h"

namespace colossusx {
namespace bench {

enum class RetentionMode {
  kFull,
  kHalf,
  kQuarter,
  kMinimal,
  kHotset10,
  kHotset25,
  kCompressedScaffold,
};

struct RetentionBenchmarkResult {
  std::string benchmark_name;
  std::string mode;
  std::uint64_t trace_accesses = 0;
  std::uint64_t unique_items = 0;
  std::uint64_t retained_items = 0;
  std::uint64_t resident_memory_bytes = 0;
  double elapsed_ms = 0.0;
  double trace_replays_per_second = 0.0;
  double item_accesses_per_second = 0.0;
  bool scaffold_only = false;
};

struct VerifierBenchmarkResult {
  std::string benchmark_name;
  std::uint64_t iterations = 0;
  std::uint64_t resident_memory_bytes = 0;
  std::uint64_t estimated_peak_ram_bytes = 0;
  double elapsed_ms = 0.0;
  double latency_ms = 0.0;
  double verifications_per_second = 0.0;
};

struct CacheRebuildBenchmarkResult {
  std::string benchmark_name;
  std::uint64_t iterations = 0;
  std::uint64_t cache_size_bytes = 0;
  double elapsed_ms = 0.0;
  double latency_ms = 0.0;
  double rebuilds_per_second = 0.0;
};

std::vector<std::uint64_t> collect_nonce_item_trace(Profile profile,
                                                    const Hash256& epoch_seed,
                                                    const Hash256& header_digest,
                                                    std::uint64_t nonce);
RetentionBenchmarkResult run_retention_benchmark(const std::string& benchmark_name,
                                                 Profile profile,
                                                 const Hash256& epoch_seed,
                                                 const std::vector<std::uint64_t>& trace,
                                                 RetentionMode mode,
                                                 std::uint32_t iterations);
VerifierBenchmarkResult run_verifier_benchmark(const std::string& benchmark_name,
                                               Profile profile,
                                               const BlockHeader& header,
                                               const Hash256& epoch_seed,
                                               const Hash256& target,
                                               std::uint64_t nonce,
                                               std::uint32_t iterations);
CacheRebuildBenchmarkResult run_cache_rebuild_benchmark(const std::string& benchmark_name,
                                                        Profile profile,
                                                        const Hash256& epoch_seed,
                                                        std::uint32_t iterations);
std::string format_retention_result(const RetentionBenchmarkResult& result);
std::string format_verifier_result(const VerifierBenchmarkResult& result);
std::string format_cache_rebuild_result(const CacheRebuildBenchmarkResult& result);

}  // namespace bench
}  // namespace colossusx

#endif
