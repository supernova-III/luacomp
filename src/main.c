#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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

static inline unsigned int hashKeyword(
    register const char *str, register size_t len) {
  static unsigned char asso_values[] = {30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 10, 10, 30,
      15, 5, 5, 15, 30, 0, 30, 10, 0, 30, 0, 10, 30, 30, 0, 30, 15, 15, 30, 0,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
  return len + asso_values[(unsigned char)str[len - 1]] +
         asso_values[(unsigned char)str[0]];
}

typedef struct {
  const char *string;
  size_t len;
  TokenType token_type;
} HashTableEntry;

#define HASH_TABLE_ENTRY(literal, type) \
  { .string = literal, .len = sizeof(literal) - 1, .token_type = type }

#define TOTAL_KEYWORDS 22
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 8
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 29
static TokenType lookupKeyword(register const char *str, register size_t len) {
  // clang-format off
  static HashTableEntry wordlist[] = {
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("in", TOKEN_IN),
    HASH_TABLE_ENTRY("nil", TOKEN_NIL),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("local", TOKEN_LOCAL),
    HASH_TABLE_ENTRY("return",TOKEN_RETURN),
    HASH_TABLE_ENTRY("if", TOKEN_IF),
    HASH_TABLE_ENTRY("for", TOKEN_FOR),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("while", TOKEN_WHILE),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("or", TOKEN_OR),
    HASH_TABLE_ENTRY("function", TOKEN_FUNCTION),
    HASH_TABLE_ENTRY("else", TOKEN_ELSE),
    HASH_TABLE_ENTRY("false", TOKEN_FALSE),
    HASH_TABLE_ENTRY("elseif",TOKEN_ELSEIF),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("not", TOKEN_NOT),
    HASH_TABLE_ENTRY("then", TOKEN_THEN),
    HASH_TABLE_ENTRY("until", TOKEN_UNTIL),
    HASH_TABLE_ENTRY("repeat", TOKEN_REPEAT),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("end", TOKEN_END),
    HASH_TABLE_ENTRY("true", TOKEN_TRUE),
    HASH_TABLE_ENTRY("break", TOKEN_BREAK),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID},
    HASH_TABLE_ENTRY("do", TOKEN_DO),
    HASH_TABLE_ENTRY("and", TOKEN_AND),
    HASH_TABLE_ENTRY("goto", TOKEN_GOTO)
  };
  // clang-format on

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
    register uint8_t key = hashKeyword(str, len);

    if (key <= MAX_HASH_VALUE) {
      const HashTableEntry *s = &wordlist[key];

      if (len == s->len && !strncmp(str, s->string, len)) return s->token_type;
    }
  }
  return TOKEN_INVALID;
}

int main() {
  return 0;
}
