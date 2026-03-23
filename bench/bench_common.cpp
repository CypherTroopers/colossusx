#include "bench/bench_common.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/epoch_cache.h"
#include "core/get_item.h"
#include "core/pow.h"
#include "core/round_function.h"
#include "core/scratch.h"
#include "core/verifier.h"

namespace colossusx {
namespace bench {
namespace {

using Clock = std::chrono::steady_clock;

std::string retention_mode_name(RetentionMode mode) {
  switch (mode) {
    case RetentionMode::kFull:
      return "full_retention";
    case RetentionMode::kHalf:
      return "half_retention";
    case RetentionMode::kQuarter:
      return "quarter_retention";
    case RetentionMode::kMinimal:
      return "minimal_retention";
    case RetentionMode::kHotset10:
      return "hotset_10";
    case RetentionMode::kHotset25:
      return "hotset_25";
    case RetentionMode::kCompressedScaffold:
      return "compressed_scaffold";
  }
  throw std::invalid_argument("unknown retention mode");
}

std::vector<std::uint64_t> select_retained_items(const std::vector<std::uint64_t>& trace,
                                                 RetentionMode mode) {
  std::unordered_map<std::uint64_t, std::uint64_t> counts;
  std::vector<std::uint64_t> first_seen;
  for (std::uint64_t item_index : trace) {
    const auto inserted = counts.emplace(item_index, 0U);
    if (inserted.second) {
      first_seen.push_back(item_index);
    }
    ++counts[item_index];
  }

  if (mode == RetentionMode::kMinimal) {
    return {};
  }
  if (mode == RetentionMode::kCompressedScaffold) {
    return first_seen;
  }
  if (mode == RetentionMode::kFull) {
    return first_seen;
  }

  std::vector<std::pair<std::uint64_t, std::uint64_t>> hot_items;
  hot_items.reserve(counts.size());
  for (const auto& entry : counts) {
    hot_items.push_back(entry);
  }
  std::stable_sort(hot_items.begin(), hot_items.end(), [](const auto& lhs, const auto& rhs) {
    if (lhs.second != rhs.second) {
      return lhs.second > rhs.second;
    }
    return lhs.first < rhs.first;
  });

  double fraction = 0.0;
  switch (mode) {
    case RetentionMode::kHalf:
      fraction = 0.50;
      break;
    case RetentionMode::kQuarter:
      fraction = 0.25;
      break;
    case RetentionMode::kHotset10:
      fraction = 0.10;
      break;
    case RetentionMode::kHotset25:
      fraction = 0.25;
      break;
    default:
      break;
  }

  const std::size_t retain_count = std::max<std::size_t>(1U, static_cast<std::size_t>(hot_items.size() * fraction));
  std::vector<std::uint64_t> retained;
  retained.reserve(retain_count);
  for (std::size_t i = 0; i < retain_count && i < hot_items.size(); ++i) {
    retained.push_back(hot_items[i].first);
  }
  return retained;
}

std::string bool_string(bool value) {
  return value ? "true" : "false";
}

double elapsed_ms(Clock::time_point start, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

void append_kv(const std::string& key, const std::string& value, std::ostringstream* out) {
  *out << key << '=' << value << '\n';
}

void benchmark_touch_item(const DatasetItem& item, std::uint64_t* accumulator) {
  for (std::size_t offset = 0; offset < item.size(); offset += sizeof(std::uint64_t)) {
    std::uint64_t word = 0;
    for (std::size_t i = 0; i < sizeof(std::uint64_t); ++i) {
      word |= static_cast<std::uint64_t>(item[offset + i]) << (8U * i);
    }
    *accumulator ^= word;
  }
}

}  // namespace

std::vector<std::uint64_t> collect_nonce_item_trace(Profile profile,
                                                    const Hash256& epoch_seed,
                                                    const Hash256& header_digest,
                                                    std::uint64_t nonce) {
  const ProfileParameters& parameters = profile_parameters(profile);
  std::vector<std::uint64_t> trace;
  trace.reserve(static_cast<std::size_t>(parameters.lanes) * parameters.rounds * 3U);

  for (std::uint32_t lane_id = 0; lane_id < parameters.lanes; ++lane_id) {
    LaneScratch scratch = LaneScratch::InitializeLazy(profile, epoch_seed, header_digest, nonce, lane_id);
    LaneState state = initialize_lane_state(profile, epoch_seed, header_digest, nonce, lane_id);
    for (std::uint32_t round = 0; round < parameters.rounds; ++round) {
      const RoundTrace trace_step = execute_round(profile, epoch_seed, round, state, &scratch);
      trace.push_back(trace_step.item_index0);
      trace.push_back(trace_step.item_index1);
      trace.push_back(trace_step.item_index2);
      state = trace_step.next_state;
    }
  }
  return trace;
}

RetentionBenchmarkResult run_retention_benchmark(const std::string& benchmark_name,
                                                 Profile profile,
                                                 const Hash256& epoch_seed,
                                                 const std::vector<std::uint64_t>& trace,
                                                 RetentionMode mode,
                                                 std::uint32_t iterations) {
  if (iterations == 0U) {
    throw std::invalid_argument("retention benchmark requires at least one iteration");
  }

  const std::vector<std::uint64_t> retained_indices = select_retained_items(trace, mode);
  std::unordered_map<std::uint64_t, DatasetItem> retained_items;
  retained_items.reserve(retained_indices.size());
  if (mode != RetentionMode::kCompressedScaffold) {
    for (std::uint64_t item_index : retained_indices) {
      retained_items.emplace(item_index, get_item(profile, epoch_seed, item_index));
    }
  }

  std::uint64_t sink = 0;
  const Clock::time_point start = Clock::now();
  for (std::uint32_t iteration = 0; iteration < iterations; ++iteration) {
    for (std::uint64_t item_index : trace) {
      if (mode == RetentionMode::kCompressedScaffold) {
        benchmark_touch_item(get_item(profile, epoch_seed, item_index), &sink);
        continue;
      }
      const auto found = retained_items.find(item_index);
      if (found != retained_items.end()) {
        benchmark_touch_item(found->second, &sink);
      } else {
        benchmark_touch_item(get_item(profile, epoch_seed, item_index), &sink);
      }
    }
  }
  const Clock::time_point end = Clock::now();
  (void)sink;

  std::unordered_set<std::uint64_t> unique(trace.begin(), trace.end());
  RetentionBenchmarkResult result;
  result.benchmark_name = benchmark_name;
  result.mode = retention_mode_name(mode);
  result.trace_accesses = trace.size();
  result.unique_items = unique.size();
  result.retained_items = retained_indices.size();
  result.resident_memory_bytes = retained_indices.size() * kDatasetItemBytes;
  result.elapsed_ms = elapsed_ms(start, end);
  result.trace_replays_per_second = (1000.0 * iterations) / result.elapsed_ms;
  result.item_accesses_per_second = (1000.0 * static_cast<double>(trace.size()) * iterations) / result.elapsed_ms;
  result.scaffold_only = (mode == RetentionMode::kCompressedScaffold);
  return result;
}

VerifierBenchmarkResult run_verifier_benchmark(const std::string& benchmark_name,
                                               Profile profile,
                                               const BlockHeader& header,
                                               const Hash256& epoch_seed,
                                               const Hash256& target,
                                               std::uint64_t nonce,
                                               std::uint32_t iterations) {
  if (iterations == 0U) {
    throw std::invalid_argument("verifier benchmark requires at least one iteration");
  }

  VerifierInput input;
  input.profile = profile;
  input.header = header;
  input.nonce = nonce;
  input.target = target;
  input.epoch_seed = epoch_seed;
  input.epoch_cache = build_epoch_cache(profile, epoch_seed);

  const Clock::time_point start = Clock::now();
  for (std::uint32_t i = 0; i < iterations; ++i) {
    const VerifierResult result = verify_pow(input);
    if (!result.target_met && target == input.target && target[0] == 0xffU) {
      throw std::runtime_error("verifier benchmark expected permissive target to pass");
    }
  }
  const Clock::time_point end = Clock::now();

  const ProfileParameters& parameters = profile_parameters(profile);
  VerifierBenchmarkResult result;
  result.benchmark_name = benchmark_name;
  result.iterations = iterations;
  result.resident_memory_bytes = parameters.cache_size_bytes;
  result.estimated_peak_ram_bytes = parameters.cache_size_bytes + parameters.scratch_total_bytes + (64ULL << 20U);
  result.elapsed_ms = elapsed_ms(start, end);
  result.latency_ms = result.elapsed_ms / iterations;
  result.verifications_per_second = (1000.0 * iterations) / result.elapsed_ms;
  return result;
}

CacheRebuildBenchmarkResult run_cache_rebuild_benchmark(const std::string& benchmark_name,
                                                        Profile profile,
                                                        const Hash256& epoch_seed,
                                                        std::uint32_t iterations) {
  if (iterations == 0U) {
    throw std::invalid_argument("cache rebuild benchmark requires at least one iteration");
  }

  const Clock::time_point start = Clock::now();
  for (std::uint32_t i = 0; i < iterations; ++i) {
    EpochCache cache;
    rebuild_epoch_cache(profile, epoch_seed, &cache);
  }
  const Clock::time_point end = Clock::now();

  CacheRebuildBenchmarkResult result;
  result.benchmark_name = benchmark_name;
  result.iterations = iterations;
  result.cache_size_bytes = profile_parameters(profile).cache_size_bytes;
  result.elapsed_ms = elapsed_ms(start, end);
  result.latency_ms = result.elapsed_ms / iterations;
  result.rebuilds_per_second = (1000.0 * iterations) / result.elapsed_ms;
  return result;
}

std::string format_retention_result(const RetentionBenchmarkResult& result) {
  std::ostringstream out;
  append_kv("benchmark", result.benchmark_name, &out);
  append_kv("mode", result.mode, &out);
  append_kv("trace_accesses", std::to_string(result.trace_accesses), &out);
  append_kv("unique_items", std::to_string(result.unique_items), &out);
  append_kv("retained_items", std::to_string(result.retained_items), &out);
  append_kv("resident_memory_bytes", std::to_string(result.resident_memory_bytes), &out);
  append_kv("elapsed_ms", std::to_string(result.elapsed_ms), &out);
  append_kv("trace_replays_per_second", std::to_string(result.trace_replays_per_second), &out);
  append_kv("item_accesses_per_second", std::to_string(result.item_accesses_per_second), &out);
  append_kv("scaffold_only", bool_string(result.scaffold_only), &out);
  return out.str();
}

std::string format_verifier_result(const VerifierBenchmarkResult& result) {
  std::ostringstream out;
  append_kv("benchmark", result.benchmark_name, &out);
  append_kv("iterations", std::to_string(result.iterations), &out);
  append_kv("resident_memory_bytes", std::to_string(result.resident_memory_bytes), &out);
  append_kv("estimated_peak_ram_bytes", std::to_string(result.estimated_peak_ram_bytes), &out);
  append_kv("elapsed_ms", std::to_string(result.elapsed_ms), &out);
  append_kv("latency_ms", std::to_string(result.latency_ms), &out);
  append_kv("verifications_per_second", std::to_string(result.verifications_per_second), &out);
  return out.str();
}

std::string format_cache_rebuild_result(const CacheRebuildBenchmarkResult& result) {
  std::ostringstream out;
  append_kv("benchmark", result.benchmark_name, &out);
  append_kv("iterations", std::to_string(result.iterations), &out);
  append_kv("cache_size_bytes", std::to_string(result.cache_size_bytes), &out);
  append_kv("elapsed_ms", std::to_string(result.elapsed_ms), &out);
  append_kv("latency_ms", std::to_string(result.latency_ms), &out);
  append_kv("rebuilds_per_second", std::to_string(result.rebuilds_per_second), &out);
  return out.str();
}

}  // namespace bench
}  // namespace colossusx
