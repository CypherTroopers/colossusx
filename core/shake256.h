#ifndef COLOSSUSX_CORE_SHAKE256_H_
#define COLOSSUSX_CORE_SHAKE256_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace colossusx {

std::vector<std::uint8_t> shake256(const std::vector<std::uint8_t>& input,
                                   std::size_t output_size);

}  // namespace colossusx

#endif
