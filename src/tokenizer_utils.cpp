#include "tokenizer_utils.hh"

bool IsDigit(char c) {
  return c >= '0' && c <= '9';
}
bool IsAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool IsHexChar(char c) {
  return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool IsHexadecimal(char c) {
  return IsDigit(c) || IsHexChar(c);
}

double CharToDigit(char c) {
  return c - '0';
}

bool IsKeywordCharacter(char c) {
  return IsAlpha(c) || IsDigit(c) || c == '_';
}
