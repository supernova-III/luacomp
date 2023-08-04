#include "lib.cpp"
#include "tokenizer.cpp"
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

#define STR(s) s, sizeof(s) - 1

TEST(Tokenizer, ScanNumber) {
  size_t len = 0;
  auto res = ScanNumber(STR("123   "), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 3);

  res = ScanNumber(STR("123.123   "), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 7);

  res = ScanNumber(STR("123.123e+10   "), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 11);

  res = ScanNumber(STR(".123e+10   "), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 8);

  res = ScanNumber(STR(".e+10   "), len);
  EXPECT_EQ(res, ScanNumberError::NO_NUMBER);

  res = ScanNumber(STR(".123e+10   "), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 8);

  res = ScanNumber(STR("123.e-10   "), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 8);

  res = ScanNumber(STR("123.e-10 asdasd"), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 8);

  res = ScanNumber(STR("0x123.e-10 asdasd"), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 7);

  res = ScanNumber(STR("0xabcef.effp-10 asdasd"), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 15);

  res = ScanNumber(STR("0xabcef.effp+10 asdasd"), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 15);

  res = ScanNumber(STR("0xabcef.effp10 asdasd"), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 14);

  res = ScanNumber(STR("0x.effp10 asdasd"), len);
  EXPECT_EQ(res, ScanNumberError::OK);
  EXPECT_EQ(len, 9);
}

TEST(Tokenizer, EvaluateNumber) {
  auto res = EvaluateNumber(
      STR("123"), NumberBase::Enum::DEC, CharToDigit, 3, ExponentType{}, 3);
  EXPECT_EQ(res, 123);

  res = EvaluateNumber(
      STR("123.123"), NumberBase::Enum::DEC, CharToDigit, 3, ExponentType{}, 7);
  EXPECT_EQ(res, 123.123);

  res = EvaluateNumber(
      STR(".123"), NumberBase::Enum::DEC, CharToDigit, 0, ExponentType{}, 4);
  EXPECT_EQ(res, .123);

  res = EvaluateNumber(
      STR("123."), NumberBase::Enum::DEC, CharToDigit, 3, ExponentType{}, 4);
  EXPECT_EQ(res, 123.);

  res = EvaluateNumber(STR("123.1E+1"), NumberBase::Enum::DEC, CharToDigit, 3,
      ExponentType::PLUS, 5);
  EXPECT_EQ(res, 123.1e+1);

  res = EvaluateNumber(STR(".2E-3"), NumberBase::Enum::DEC, CharToDigit, 0,
      ExponentType::MINUS, 2);
  EXPECT_EQ(res, .2e-3);

  res = EvaluateNumber(
      STR("1"), NumberBase::Enum::HEX, HexToNumber, 1, ExponentType{}, 1);
  EXPECT_EQ(res, 0x1);

  res = EvaluateNumber(
      STR("ff"), NumberBase::Enum::HEX, HexToNumber, 2, ExponentType{}, 2);
  EXPECT_EQ(res, 0xff);

  res = EvaluateNumber(
      STR("f.f"), NumberBase::Enum::HEX, HexToNumber, 1, ExponentType{}, 3);
  EXPECT_EQ(res, 0xf.fp0);

  res = EvaluateNumber(STR("12f.12fp+2"), NumberBase::Enum::HEX, HexToNumber, 3,
      ExponentType::PLUS, 7);
  EXPECT_EQ(res, 0x12f.12fp2);

  res = EvaluateNumber(STR("12f.12fp-2"), NumberBase::Enum::HEX, HexToNumber, 3,
      ExponentType::MINUS, 7);
  EXPECT_EQ(res, 0x12f.12fp-2);

  res = EvaluateNumber(STR(".12fp-2"), NumberBase::Enum::HEX, HexToNumber, 0,
      ExponentType::MINUS, 4);
  EXPECT_EQ(res, 0x.12fp-2);

  res = EvaluateNumber(STR("12f.p-2"), NumberBase::Enum::HEX, HexToNumber, 3,
      ExponentType::MINUS, 4);
  EXPECT_EQ(res, 0x12f.p-2);
}
