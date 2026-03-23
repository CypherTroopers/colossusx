cmake_minimum_required(VERSION 3.16)
project(colossusx_reference LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(colossusx_core
    core/header.cpp
    core/epoch_seed.cpp
    core/epoch_cache.cpp
    core/get_item.cpp
    core/round_function.cpp
    core/scratch.cpp
    core/pow.cpp
    core/verifier.cpp
    core/shake256.cpp
)
target_include_directories(colossusx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(colossusx_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(colossusx_bench_support
    bench/bench_common.cpp
)
target_include_directories(colossusx_bench_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(colossusx_bench_support PUBLIC colossusx_core)
target_compile_options(colossusx_bench_support PRIVATE -Wall -Wextra -Wpedantic)

add_executable(bench_full_retention
    bench/bench_full_retention.cpp
)
target_link_libraries(bench_full_retention PRIVATE colossusx_bench_support)
target_compile_options(bench_full_retention PRIVATE -Wall -Wextra -Wpedantic)

add_executable(bench_half_retention
    bench/bench_half_retention.cpp
)
target_link_libraries(bench_half_retention PRIVATE colossusx_bench_support)
target_compile_options(bench_half_retention PRIVATE -Wall -Wextra -Wpedantic)

add_executable(bench_quarter_retention
    bench/bench_quarter_retention.cpp
)
target_link_libraries(bench_quarter_retention PRIVATE colossusx_bench_support)
target_compile_options(bench_quarter_retention PRIVATE -Wall -Wextra -Wpedantic)

add_executable(bench_minimal_retention
    bench/bench_minimal_retention.cpp
)
target_link_libraries(bench_minimal_retention PRIVATE colossusx_bench_support)
target_compile_options(bench_minimal_retention PRIVATE -Wall -Wextra -Wpedantic)

add_executable(bench_hotset_retention
    bench/bench_hotset_retention.cpp
)
target_link_libraries(bench_hotset_retention PRIVATE colossusx_bench_support)
target_compile_options(bench_hotset_retention PRIVATE -Wall -Wextra -Wpedantic)

add_executable(bench_compressed_retention
    bench/bench_compressed_retention.cpp
)
target_link_libraries(bench_compressed_retention PRIVATE colossusx_bench_support)
target_compile_options(bench_compressed_retention PRIVATE -Wall -Wextra -Wpedantic)

add_executable(bench_verifier
    bench/bench_verifier.cpp
)
target_link_libraries(bench_verifier PRIVATE colossusx_bench_support)
target_compile_options(bench_verifier PRIVATE -Wall -Wextra -Wpedantic)

add_library(colossusx_vector_tools
    tools/generate_test_vectors.cpp
)
target_include_directories(colossusx_vector_tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(colossusx_vector_tools PUBLIC colossusx_core)
target_compile_options(colossusx_vector_tools PRIVATE -Wall -Wextra -Wpedantic)

add_executable(generate_test_vectors
    tools/generate_test_vectors_main.cpp
)
target_link_libraries(generate_test_vectors PRIVATE colossusx_vector_tools)
target_compile_options(generate_test_vectors PRIVATE -Wall -Wextra -Wpedantic)

add_executable(colossusx_tests
    tests/test_main.cpp
    tests/test_header_digest.cpp
    tests/test_epoch_seed.cpp
    tests/test_epoch_cache.cpp
    tests/test_get_item.cpp
    tests/test_round_function.cpp
    tests/test_scratch_init.cpp
    tests/test_pow_replay.cpp
    tests/test_verifier.cpp
    tests/test_vectors.cpp
    tests/test_benchmarks.cpp
)
target_link_libraries(colossusx_tests PRIVATE colossusx_core colossusx_vector_tools colossusx_bench_support)
target_compile_options(colossusx_tests PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME colossusx_tests COMMAND colossusx_tests)
