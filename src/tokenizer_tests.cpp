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
