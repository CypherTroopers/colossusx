#include "bench/bench_common.h"
#include "tests/test_helpers.h"

#include <string>
#include <vector>

namespace {

void TestRetentionBenchmarkSmoke() {
  const colossusx::Hash256 epoch_seed{};
  const std::vector<std::uint64_t> trace = {1U, 2U, 1U, 3U, 2U, 1U};
  const auto result = colossusx::bench::run_retention_benchmark(
      "bench_smoke", colossusx::Profile::kCx18, epoch_seed, trace,
      colossusx::bench::RetentionMode::kMinimal, 1U);
  EXPECT_EQ(std::string("minimal_retention"), result.mode);
  EXPECT_EQ(6ULL, result.trace_accesses);
  EXPECT_TRUE(result.item_accesses_per_second > 0.0);
}

void TestBenchmarkFormattingSmoke() {
  colossusx::bench::CacheRebuildBenchmarkResult result;
  result.benchmark_name = "bench_cache_rebuild";
  result.iterations = 1U;
  result.cache_size_bytes = 1024U;
  result.elapsed_ms = 1.5;
  result.latency_ms = 1.5;
  result.rebuilds_per_second = 666.0;
  const std::string formatted = colossusx::bench::format_cache_rebuild_result(result);
  EXPECT_TRUE(formatted.find("benchmark=bench_cache_rebuild\n") != std::string::npos);
  EXPECT_TRUE(formatted.find("cache_size_bytes=1024\n") != std::string::npos);
}

void RunBenchmarkTests() {
  TestRetentionBenchmarkSmoke();
  TestBenchmarkFormattingSmoke();
}

}  // namespace

void run_benchmark_tests() { RunBenchmarkTests(); }
