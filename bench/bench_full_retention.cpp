#include <iostream>

#include "bench/bench_common.h"
#include "bench/bench_inputs.h"

int main() {
  const auto trace = colossusx::bench::collect_nonce_item_trace(
      colossusx::Profile::kCx18, colossusx::bench::benchmark_epoch_seed(),
      colossusx::bench::benchmark_header_digest(), colossusx::bench::benchmark_nonce());
  const auto result = colossusx::bench::run_retention_benchmark(
      "bench_full_retention", colossusx::Profile::kCx18,
      colossusx::bench::benchmark_epoch_seed(), trace,
      colossusx::bench::RetentionMode::kFull, 1U);
  std::cout << colossusx::bench::format_retention_result(result);
  return 0;
}
