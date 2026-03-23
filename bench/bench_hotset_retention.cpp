#include <iostream>

#include "bench/bench_common.h"
#include "bench/bench_inputs.h"

int main() {
  const auto trace = colossusx::bench::collect_nonce_item_trace(
      colossusx::Profile::kCx18, colossusx::bench::benchmark_epoch_seed(),
      colossusx::bench::benchmark_header_digest(), colossusx::bench::benchmark_nonce());
  const auto hot10 = colossusx::bench::run_retention_benchmark(
      "bench_hotset_retention", colossusx::Profile::kCx18,
      colossusx::bench::benchmark_epoch_seed(), trace,
      colossusx::bench::RetentionMode::kHotset10, 1U);
  const auto hot25 = colossusx::bench::run_retention_benchmark(
      "bench_hotset_retention", colossusx::Profile::kCx18,
      colossusx::bench::benchmark_epoch_seed(), trace,
      colossusx::bench::RetentionMode::kHotset25, 1U);
  std::cout << colossusx::bench::format_retention_result(hot10) << '\n'
            << colossusx::bench::format_retention_result(hot25);
  return 0;
}
