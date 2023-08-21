#include "lib.hh"
#include "tokenizer.hh"
#define GTEST_BREAK_ON_FAILURE 1
#include <gtest/gtest.h>
#include <array>

bool operator==(const Token& left, const Token& right) {
  if (left.type == right.type) {
    switch (left.type) {
      case TOKEN_IDENTIFIER:
        return left.value.identifier == right.value.identifier;
      case TOKEN_LONG_STRING_LITERAL:
      case TOKEN_SHORT_STRING_LITERAL:
        return left.value.string_literal == right.value.string_literal;
      case TOKEN_NUMBER: return left.value.number == right.value.number;
      default: return false;
    }
  }
  return false;
}

template <typename T, typename U>
struct TestCase {
  T input;
  U expected_value;
  String desc;
};

TEST(Tokenizer, TryEvaluateInteger) {
#define tok_testcase(val, description)                        \
  TestCase<String, EvaluateNumberResult> {                    \
    .input = String(#val),                                    \
    .expected_value = {val, EvaluateNumberResult::Error::OK}, \
    .desc = description                                       \
  }
  // clang-format off
  auto test_cases = std::array{
    tok_testcase(23, "Decimal integer test"),
    tok_testcase(23.123, "Decimal float test"),
    tok_testcase(23.123e12, "Decimal float with exponent test"),
    tok_testcase(23.123e+12, "Decimal float with positive exponent test"),
    tok_testcase(23.123e-12, "Decimal float with negative exponent test"),
    tok_testcase(123, "Decimal float without integer part test"),
    tok_testcase(123E123, "Decimal float without integer part with exponent test"),
    tok_testcase(123E+123, "Decimal float without integer part with positive exponent test"),
    tok_testcase(123E-123, "Decimal float without integer part with negative exponent test"),
    tok_testcase(12., "Decimal float without fractional part test"),
    tok_testcase(12.e123, "Decimal float without fractional part with exponent test"),
    tok_testcase(12.e+123, "Decimal float without fractional part with positive exponent test"),
    tok_testcase(12.e-123, "Decimal float without fractional part with negative exponent test"),
  };
  // clang-format on
  for (size_t id = 0; id < test_cases.size(); ++id) {
    auto& test_case = test_cases[id];
    StringIterator iter(test_case.input);
    const auto [number, error] = EvaluateNumber(iter);
    EXPECT_DOUBLE_EQ(test_case.expected_value.number, number)
        << "Test #" << id << ": " << test_case.desc.data;
    EXPECT_EQ(test_case.expected_value.error, error)
        << "Test #" << id << ": " << test_case.desc.data;
  }
}
