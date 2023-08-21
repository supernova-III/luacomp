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
#define tok_testcase(val, expected, description)                   \
  TestCase<String, EvaluateNumberResult> {                         \
    .input = String(val),                                          \
    .expected_value = {expected, EvaluateNumberResult::Error::OK}, \
    .desc = description                                            \
  }
  // clang-format off
  auto test_cases = std::array{
    tok_testcase("23",                  23,                     "Decimal integer"),
    tok_testcase("23.123",              23.123,                 "Decimal float"),
    tok_testcase("23.123e12",           23.123e12,              "Decimal float with exponent"),
    tok_testcase("23.123e+12",          23.123e+12,             "Decimal float with positive exponent"),
    tok_testcase("23.123e-12",          23.123e-12,             "Decimal float with negative exponent"),

    tok_testcase(".123",                .123,                   "Decimal float without integer part"),
    tok_testcase(".123E12",             .123E12,                "Decimal float without integer part with exponent"),
    tok_testcase(".123E+12",            .123e+12,               "Decimal float without integer part with positive exponent"),
    tok_testcase(".123E-12",            .123e-12,               "Decimal float without integer part with negative exponent"),

    tok_testcase("12.",                 12.,                    "Decimal float without fractional part"),
    tok_testcase("12.e12",              12.e12,                 "Decimal float without fractional part with exponent"),
    tok_testcase("12.e+12",             12.e+12,                "Decimal float without fractional part with positive exponent"),
    tok_testcase("12.e-12",             12.e-12,                "Decimal float without fractional part with negative exponent"),

    tok_testcase("0x1234567890abcdef",  0x1234567890abcdef.p0,  "Hexadecimal integer"),
    tok_testcase("0x1234567890.abcdef", 0x1234567890.abcdefp0,  "Hexadecimal float"),
    tok_testcase("0x1234567890abcdef.", 0x1234567890abcdef.p0,  "Hexadecimal float without fractional part"),
    tok_testcase("0x.1234567890abcdef", 0x.1234567890abcdefp0,  "Hexadecimal float without integer part"),

    tok_testcase("0x123abcp2",          0x123abcp2,             "Hexadecimal integer with exponent"),
    tok_testcase("0x123abc.123abcp2",   0x123abc.123abcp2,      "Hexadecimal float with exponent"),
    tok_testcase("0x.123abcp2",         0x.123abcp2,            "Hexadecimal float without integer part and with exponent"),
    tok_testcase("0x123abc.p2",         0x123abc.p2,            "Hexadecimal float without fractional part and with exponent"),

    tok_testcase("0x123abcp+2",         0x123abcp2,             "Hexadecimal integer with positive exponent"),
    tok_testcase("0x123abc.123abcp+2",  0x123abc.123abcp2,      "Hexadecimal float with positive exponent"),
    tok_testcase("0x.123abcp-2",        0x.123abcp-2,           "Hexadecimal float without integer part and with negative exponent"),
    tok_testcase("0x123abc.p-2",        0x123abc.p-2,           "Hexadecimal float without fractional part and with negative exponent"),
  };
  // clang-format on
#undef tok_testcase
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
