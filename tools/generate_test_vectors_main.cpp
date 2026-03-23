#include "tools/generate_test_vectors.h"

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  colossusx::vectors::VectorPreset preset = colossusx::vectors::VectorPreset::kCanonicalFreeze;
  int path_index = 1;
  if (argc >= 2 && std::string(argv[1]) == "--smoke") {
    preset = colossusx::vectors::VectorPreset::kSmoke;
    path_index = 2;
  }

  const std::string document = colossusx::vectors::generate_test_vectors_document(preset);
  if (argc > path_index) {
    std::ofstream output(argv[path_index], std::ios::binary);
    output << document;
    return output ? 0 : 1;
  }
  std::cout << document;
  return 0;
}
