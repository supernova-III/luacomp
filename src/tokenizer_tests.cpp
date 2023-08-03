#include "lib.cpp"
#include "tokenizer.cpp"
#include <gtest/gtest.h>

TEST(Tokenizer, Creation) {
  const char program[] = R"(
    function print(...)
    end
  )";
  auto iter = TokenIterator("", program, sizeof(program));
  EXPECT_FALSE(iter);
  LuaTokenType expected_tokens[] = {TOKEN_FUNCTION, TOKEN_IDENTIFIER,
      TOKEN_LEFT_PAREN, TOKEN_3PERIOD, TOKEN_RIGHT_PAREN, TOKEN_END};
  for (const auto tok : expected_tokens) {
    ++iter;
    ASSERT_EQ(tok, (*iter).type);
  }
  ++iter;
  EXPECT_FALSE(iter);
}

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

struct NumberScanTestData {
  Token expected;
  TokenIterator iterator;
};

NumberScanTestData numberScanTestBase(double expected_value, const char* str) {
  const size_t len = strlen(str);
  Token expected = {.type = TOKEN_NUMBER,
      .value = decltype(Token::value){.number = expected_value}};
  return {expected, TokenIterator("", str, len)};
}

TEST(Tokenizer, BasicIntegerScan) {
  {
    auto [expected, iter] = numberScanTestBase(12345, "12345");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }

  {
    auto [expected, iter] = numberScanTestBase(12345, "00012345");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }

  {
    auto [expected, iter] = numberScanTestBase(0, "000x12345");
    ++iter;
    ASSERT_EQ(expected, *iter);
    ++iter;
    ASSERT_EQ(TOKEN_IDENTIFIER, (*iter).type);
  }
}

TEST(Tokenizer, BasicFloatScan_Decimal) {
  auto [expected, iter] = numberScanTestBase(123.45, "123.45");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScanWithEmptyIntegerPart_Decimal) {
  auto [expected, iter] = numberScanTestBase(.45, ".45");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScanWithEmptyFractionalPart_Decimal) {
  auto [expected, iter] = numberScanTestBase(45., "45.");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScanWithExponent_Decimal) {
  {
    auto [expected, iter] = numberScanTestBase(45.1e10, "45.1e10");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }
  {
    auto [expected, iter] = numberScanTestBase(45.1000e-10, "45.1000e-10");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }
  {
    auto [expected, iter] = numberScanTestBase(45.1e+10, "45.1e+10");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }
}

TEST(Tokenizer, BasicFloatScanWithExponentAndEmptyIntegerPart_Decimal) {
  auto [expected, iter] = numberScanTestBase(.123e10, ".123e10");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScanWithExponentAndEmptyFractionalPart_Decimal) {
  auto [expected, iter] = numberScanTestBase(123.e10, "123.e10");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicIntegerScan_Hex) {
  {
    auto [expected, iter] = numberScanTestBase(0x123, "0x123");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }
  auto [expected, iter] = numberScanTestBase(0x123, "0x00000123");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScan_Hex) {
  auto [expected, iter] =
      numberScanTestBase(0x123ab.123abep0, "0x123ab.123abe");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScanWithoutFractionalPart_Hex) {
  auto [expected, iter] = numberScanTestBase(0x123.p0, "0x123.");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScanWithoutIntegerPart_Hex) {
  auto [expected, iter] = numberScanTestBase(0x.123p0, "0x.123");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScanWithExponent_Hex) {
  {
    auto [expected, iter] = numberScanTestBase(0x123.1bep12, "0x123.1bep12");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }
  {
    auto [expected, iter] = numberScanTestBase(0x123.1bep-12, "0x123.1bep-12");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }
  {
    auto [expected, iter] = numberScanTestBase(0x123.1bep+12, "0x123.1bep+12");
    ++iter;
    ASSERT_EQ(expected, *iter);
  }
}

TEST(Tokenizer, BasicFloatScanWithExponentWithoutIntegerPart_Hex) {
  auto [expected, iter] = numberScanTestBase(0x.123p12, "0x.123p12");
  ++iter;
  ASSERT_EQ(expected, *iter);
}

TEST(Tokenizer, BasicFloatScanWithExponentWithoutFloatPart_Hex) {
  auto [expected, iter] = numberScanTestBase(0x123.p12, "0x123.p12");
  ++iter;
  ASSERT_EQ(expected, *iter);
}
