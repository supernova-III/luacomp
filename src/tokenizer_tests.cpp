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
