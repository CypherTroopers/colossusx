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
    core/shake256.cpp
)
target_include_directories(colossusx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(colossusx_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(colossusx_tests
    tests/test_main.cpp
    tests/test_header_digest.cpp
    tests/test_epoch_seed.cpp
    tests/test_epoch_cache.cpp
    tests/test_get_item.cpp
    tests/test_round_function.cpp
    tests/test_scratch_init.cpp
)
target_link_libraries(colossusx_tests PRIVATE colossusx_core)
target_compile_options(colossusx_tests PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME colossusx_tests COMMAND colossusx_tests)
