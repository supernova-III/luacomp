#include "lib.cpp"
#include "tokenizer.cpp"
#include <gtest/gtest.h>

TEST(Tokenizer, Creation) {
  const auto program = R"(
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
}
