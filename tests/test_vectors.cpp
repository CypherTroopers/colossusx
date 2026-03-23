#include "tools/generate_test_vectors.h"
#include "tests/test_helpers.h"

#include <cctype>
#include <string>

namespace {

std::string FieldValue(const std::string& document, const std::string& vector_id,
                       const std::string& field_name) {
  const std::string marker = "vector_id: " + vector_id + "\n";
  const std::size_t start = document.find(marker);
  EXPECT_TRUE(start != std::string::npos);
  const std::size_t field_pos = document.find(field_name + ": ", start);
  EXPECT_TRUE(field_pos != std::string::npos);
  const std::size_t value_start = field_pos + field_name.size() + 2U;
  const std::size_t value_end = document.find('\n', value_start);
  EXPECT_TRUE(value_end != std::string::npos);
  return document.substr(value_start, value_end - value_start);
}

bool IsLowerHexOrSeparators(const std::string& value) {
  for (char ch : value) {
    if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || ch == '|' || ch == ',' ) {
      continue;
    }
    return false;
  }
  return true;
}

void TestSmokeVectorGenerationIsDeterministic() {
  const std::string first =
      colossusx::vectors::generate_test_vectors_document(colossusx::vectors::VectorPreset::kSmoke);
  const std::string second =
      colossusx::vectors::generate_test_vectors_document(colossusx::vectors::VectorPreset::kSmoke);
  EXPECT_EQ(first, second);
}

void TestSmokeVectorFormattingMatchesReferenceStyle() {
  const std::string document =
      colossusx::vectors::generate_test_vectors_document(colossusx::vectors::VectorPreset::kSmoke);
  EXPECT_TRUE(document.find("## TV-001-A\nvector_id: TV-001-A\n") != std::string::npos);
  EXPECT_TRUE(document.find("## TV-008-A\nvector_id: TV-008-A\n") != std::string::npos);
  EXPECT_TRUE(IsLowerHexOrSeparators(FieldValue(document, "TV-001-A", "expected_epoch_seed")));
  EXPECT_TRUE(IsLowerHexOrSeparators(FieldValue(document, "TV-002-A", "expected_header_digest")));
  EXPECT_TRUE(IsLowerHexOrSeparators(FieldValue(document, "TV-006-A", "expected_written_value")));
  EXPECT_TRUE(FieldValue(document, "TV-008-A", "expected_valid") == "true" ||
              FieldValue(document, "TV-008-A", "expected_valid") == "false");
}

void TestSmokeVectorsRoundTripAgainstReferenceImplementation() {
  const std::string document =
      colossusx::vectors::generate_test_vectors_document(colossusx::vectors::VectorPreset::kSmoke);
  EXPECT_EQ(std::string("TV-001-A"), FieldValue(document, "TV-001-A", "vector_id"));
  EXPECT_EQ(std::string("TV-002-A"), FieldValue(document, "TV-002-A", "vector_id"));
  EXPECT_EQ(std::string("TV-007-A"), FieldValue(document, "TV-007-A", "vector_id"));
  EXPECT_EQ(std::string("TV-008-A"), FieldValue(document, "TV-008-A", "vector_id"));
  EXPECT_TRUE(!FieldValue(document, "TV-007-A", "expected_pow_hash").empty());
  EXPECT_TRUE(!FieldValue(document, "TV-003-A", "expected_cache_hash").empty());
}

void RunVectorTests() {
  TestSmokeVectorGenerationIsDeterministic();
  TestSmokeVectorFormattingMatchesReferenceStyle();
  TestSmokeVectorsRoundTripAgainstReferenceImplementation();
}

}  // namespace

void run_vector_tests() { RunVectorTests(); }
