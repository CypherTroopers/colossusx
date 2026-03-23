#include <iostream>

void run_epoch_seed_tests();
void run_epoch_cache_tests();
void run_get_item_tests();
void run_round_function_tests();
void run_scratch_init_tests();
void run_pow_replay_tests();
void run_verifier_tests();
void run_vector_tests();
void run_benchmark_tests();
void run_header_digest_tests();

int main() {
  run_header_digest_tests();
  run_epoch_seed_tests();
  run_epoch_cache_tests();
  run_get_item_tests();
  run_round_function_tests();
  run_scratch_init_tests();
  run_pow_replay_tests();
  run_verifier_tests();
  run_vector_tests();
  run_benchmark_tests();
  std::cout << "all tests passed\n";
  return 0;
}
