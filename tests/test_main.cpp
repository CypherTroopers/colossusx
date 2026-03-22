#include <iostream>

void run_epoch_seed_tests();
void run_epoch_cache_tests();
void run_get_item_tests();
void run_round_function_tests();
void run_scratch_init_tests();
void run_header_digest_tests();

int main() {
  run_header_digest_tests();
  run_epoch_seed_tests();
  run_epoch_cache_tests();
  run_get_item_tests();
  run_round_function_tests();
  run_scratch_init_tests();
  std::cout << "all tests passed\n";
  return 0;
}
