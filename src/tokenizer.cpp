#include <string.h>
#include "tokenizer.hh"

TokenIterator TokenIterator::New(const char* input) {
  return New(input, strlen(input));
}

TokenIterator TokenIterator::New(const char* input, usize size) {
  return TokenIterator{.input_ = input, .input_size_ = size};
}

TokenIterator::operator bool() const {
  return current_token_.type != TOKEN_INVALID && input_pos_ < input_size_;
}

TokenIterator& TokenIterator::operator++() {
  scanForNextToken();
  return *this;
}

const Token& TokenIterator::operator*() const {
  return current_token_;
}

namespace {

inline bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

inline bool isHexChar(char c) {
  return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline bool isHexadecimal(char c) {
  return isDigit(c) || isHexChar(c);
}

inline bool isKeywordCharacter(char c) {
  return isAlpha(c) || isDigit(c) || c == '_';
}

inline double charToDigit(char c) {
  return c - '0';
}

inline double hexToNumber(char c) {
  if (isHexChar(c)) {
    if (c >= 'a') {
      c = c - ('A' - 'a');
    }
    return c - 'A' + 10;
  }
  return charToDigit(c);
}

}  // namespace

void TokenIterator::scanForNextToken() {}
