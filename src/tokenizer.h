#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
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
} TokenType;

typedef struct {
  size_t len;
  char str[];
} StringView;

typedef struct {
  TokenType type;
  union {
    StringView *identifier;
    int64_t int_literal;
    double float_literal;
    StringView *long_string_literal;
    StringView *short_string_literal;
  } value;
} Token;

typedef struct TokenIterator TokenIterator;

void InitTokenizer(const char *input, size_t len);
const Token *NextToken();
const Token *PeekToken();
