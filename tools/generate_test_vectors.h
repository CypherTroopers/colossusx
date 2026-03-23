#ifndef COLOSSUSX_TOOLS_GENERATE_TEST_VECTORS_H_
#define COLOSSUSX_TOOLS_GENERATE_TEST_VECTORS_H_

#include <string>

namespace colossusx {
namespace vectors {

enum class VectorPreset {
  kSmoke,
  kCanonicalFreeze,
};

std::string generate_test_vectors_document(VectorPreset preset);

}  // namespace vectors
}  // namespace colossusx

#endif
