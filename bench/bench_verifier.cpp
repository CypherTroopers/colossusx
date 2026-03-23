#include <iostream>

#include "bench/bench_common.h"
#include "bench/bench_inputs.h"

int main() {
  colossusx::Hash256 permissive_target{};
  permissive_target.fill(0xffU);
  const auto verifier = colossusx::bench::run_verifier_benchmark(
      "bench_verifier", colossusx::Profile::kCx18, colossusx::bench::benchmark_header(),
      colossusx::bench::benchmark_epoch_seed(), permissive_target,
      colossusx::bench::benchmark_nonce(), 1U);
  const auto cache = colossusx::bench::run_cache_rebuild_benchmark(
      "bench_cache_rebuild", colossusx::Profile::kCx18,
      colossusx::bench::benchmark_epoch_seed(), 1U);
  std::cout << colossusx::bench::format_verifier_result(verifier) << '\n'
            << colossusx::bench::format_cache_rebuild_result(cache);
  return 0;
}
