#pragma once
#include "common.hh"

enum LuaTokenType {
  TOKEN_AND,
  TOKEN_BREAK,
  TOKEN_DO,
  TOKEN_ELSE,
  TOKEN_ELSEIF,
  TOKEN_END,
  TOKEN_FALSE,
  TOKEN_FOR,
  TOKEN_FUNCTION,
  TOKEN_GOTO,
  TOKEN_IF,
  TOKEN_IN,
  TOKEN_LOCAL,
  TOKEN_NIL,
  TOKEN_NOT,
  TOKEN_OR,
  TOKEN_REPEAT,
  TOKEN_RETURN,
  TOKEN_THEN,
  TOKEN_TRUE,
  TOKEN_UNTIL,
  TOKEN_WHILE,
  KEYWORDS__COUNT,
  TOKEN_INVALID,
  TOKEN_PLUS = '+',           // ok
  TOKEN_MINUS,                // ok
  TOKEN_ASTERISK = '*',       // ok
  TOKEN_DIVIDE = '/',         // ok
  TOKEN_MOD = '%',            // ok
  TOKEN_BXOR = '^',           // ok
  TOKEN_DASH = '#',           // ok
  TOKEN_AT = '&',             // ok
  TOKEN_BNOT = '~',           // ok
  TOKEN_BOR = '|',            // ok
  TOKEN_BLEFT,                // ok
  TOKEN_BRIGHT,               // ok
  TOKEN_DIV,                  // ok
  TOKEN_EQUALS,               // ok
  TOKEN_BNOT_ASSIGN,          // ok
  TOKEN_LESS_EQUAL,           // ok
  TOKEN_BIGGER_EQUAL,         // ok
  TOKEN_LESS = '<',           // ok
  TOKEN_BIGGER = '>',         // ok
  TOKEN_ASSIGN = '=',         // ok
  TOKEN_LEFT_PAREN = '(',     // ok
  TOKEN_RIGHT_PAREN = ')',    // ok
  TOKEN_LEFT_BRACE = '{',     // ok
  TOKEN_RIGHT_BRACE = '}',    // ok
  TOKEN_LEFT_BRACKET = '[',   // ok
  TOKEN_RIGHT_BRACKET = ']',  // ok
  TOKEN_COLON_COLON,          // ok
  TOKEN_SEMICOLON = ';',      // ok
  TOKEN_COLON = ':',          // ok
  TOKEN_COMMA = ',',          // ok
  TOKEN_PERIOD,               // ok
  TOKEN_2PERIOD,              // ok
  TOKEN_3PERIOD,              // ok
  TOKEN_LONG_STRING_LITERAL,
  TOKEN_SHORT_STRING_LITERAL,
  TOKEN_NUMBER,
  TOKEN_IDENTIFIER,
  TOKEN_END_OF_STREAM  // ok
};

// It would be nice to have a token position inside the line of code and the
// line number, if applicable. But it's rather not to be stored here, because
// this data is needed only when some error comes in.
struct Token {
  LuaTokenType type = TOKEN_INVALID;
  union {
    const char* identifier;
    const char* string_literal;
    f64 number;
  } value;
};

struct TokenIterator {
  const char* input_ = nullptr;
  usize input_size_ = 0;
  usize input_pos_ = 0;
  Token current_token_ = {};

  // Should be used to create tokenizer for an input string.
  static TokenIterator New(const char* input);

  static TokenIterator New(const char* input, usize size);

  // Scan for the next token
  TokenIterator& operator++();

  // Peek the latest scanned token
  const Token& operator*() const;

  // Iterator is valid if an input stream is not exhausted or the latest scanned
  // token was not invalid
  operator bool() const;

 private:
  // The actual funcation that does tokenization
  void scanForNextToken();
};
