#ifndef COLOSSUSX_TESTS_TEST_HELPERS_H_
#define COLOSSUSX_TESTS_TEST_HELPERS_H_

#include <cstdlib>
#include <iostream>
#include <string>

#define EXPECT_TRUE(condition)                                                      \
  do {                                                                              \
    if (!(condition)) {                                                             \
      std::cerr << "EXPECT_TRUE failed at " << __FILE__ << ":" << __LINE__      \
                << ": " #condition << std::endl;                                  \
      std::exit(1);                                                                 \
    }                                                                               \
  } while (false)

#define EXPECT_EQ(expected, actual)                                                 \
  do {                                                                              \
    const auto expected_value = (expected);                                         \
    const auto actual_value = (actual);                                             \
    if (!(expected_value == actual_value)) {                                        \
      std::cerr << "EXPECT_EQ failed at " << __FILE__ << ":" << __LINE__        \
                << "\n  expected: " << expected_value                              \
                << "\n  actual:   " << actual_value << std::endl;                \
      std::exit(1);                                                                 \
    }                                                                               \
  } while (false)

#endif
