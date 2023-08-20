#include "lib.hh"
#include "tokenizer.hh"
#include <gtest/gtest.h>

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

bool operator==(
    const EvaluateNumberResult& lhs, const EvaluateNumberResult& rhs) {
  bool res = lhs.number == rhs.number;
  res = res && (lhs.len == rhs.len);
  res = res && (lhs.error == rhs.error);
  return res;
}

#define STR(s) s, sizeof(s) - 1

TEST(Tokenizer, TryEvaluateNumber) {
  auto res = TryEvaluateNumber(STR("123   "));
  EXPECT_EQ(res, EvaluateNumberResult(123, 3));

  res = TryEvaluateNumber(STR("123.123   "));
  EXPECT_EQ(res, EvaluateNumberResult(123.123, 7));

  res = TryEvaluateNumber(STR("123.123e+10   "));
  EXPECT_EQ(res, EvaluateNumberResult(123.123e+10, 11));

  res = TryEvaluateNumber(STR(".123e+10   "));
  EXPECT_EQ(res, EvaluateNumberResult(.123e+10, 8));

  res = TryEvaluateNumber(STR(".e+10   "));
  EXPECT_FALSE(res);

  res = TryEvaluateNumber(STR("12.e-10   "));
  EXPECT_EQ(res, EvaluateNumberResult(12.e-10, 7));

  res = TryEvaluateNumber(STR("12.e-10 asdasd"));
  EXPECT_EQ(res, EvaluateNumberResult(12.e-10, 7));

  res = TryEvaluateNumber(STR("0x123.e-10 asdasd"));
  EXPECT_EQ(res, EvaluateNumberResult(0x123.ep0, 7));

  res = TryEvaluateNumber(STR("0xabcef.effp-10 asdasd"));
  EXPECT_EQ(res, EvaluateNumberResult(0xabcef.effp-10, 15));

  res = TryEvaluateNumber(STR("0xabcef.effp+10 asdasd"));
  EXPECT_EQ(res, EvaluateNumberResult(0xabcef.effp+10, 15));

  res = TryEvaluateNumber(STR("0xabcef.effp10 asdasd"));
  EXPECT_EQ(res, EvaluateNumberResult(0xabcef.effp10, 14));

  res = TryEvaluateNumber(STR("0x.effp10 asdasd"));
  EXPECT_EQ(res, EvaluateNumberResult(0x.effp10, 9));
}
