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
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_ASTERISK,
  TOKEN_DIVIDE,
  TOKEN_MOD,
  TOKEN_BXOR,
  TOKEN_DASH,
  TOKEN_AT,
  TOKEN_BNOT,
  TOKEN_BOR,
  TOKEN_BLEFT,
  TOKEN_BRIGHT,
  TOKEN_DIV,
  TOKEN_SPACE,
  TOKEN_EQUALS,
  TOKEN_BXOR_ASSIGN,
  TOKEN_LESS_EQUAL,
  TOKEN_BIGGER_EQUAL,
  TOKEN_LESS,
  TOKEN_BIGGER,
  TOKEN_ASSIGN,
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_BRACKET,
  TOKEN_RIGHT_BRACKET,
  TOKEN_COLON_COLON,
  TOKEN_SEMICOLON,
  TOKEN_COLON,
  TOKEN_COMMA,
  TOKEN_PERIOD,
  TOKEN_2PERIOD,
  TOKEN_3PERIOD
} TokenType;

typedef struct {
  TokenType type;
  union {
    const char *identifier;
    int64_t int_literal;
    double float_literal;
    const char *string_literal;
    const char *short_literal;
  } value;
} Token;

typedef struct TokenIterator TokenIterator;

TokenIterator *InitTokenizer();
Token *NextToken(TokenIterator *iterator);
Token *PeekToken(const TokenIterator *iterator);
