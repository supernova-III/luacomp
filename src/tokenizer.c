#include <string.h>
#include "tokenizer.h"

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

typedef struct TokenIterator {
  const char *input;
  size_t len;
  const char *it;
  Token current;
} TokenIterator;

static TokenIterator iterator = {.current.type = TOKEN_INVALID};

void InitTokenizer(const char *input, size_t len) {
  iterator.input = input;
  iterator.len = len;
  iterator.it = input;
}

static inline void advanceInputIterator() {
  if (iterator.it - iterator.input < iterator.len) {
    ++iterator.it;
  }
}

static inline char getNextCharacter() {
  advanceInputIterator();
  return *iterator.it;
}

static inline char peekCharacter() {
  return *iterator.it;
}

const Token *PeekToken() {
  return &iterator.current;
}

const Token *NextToken() {
  char c = peekCharacter();

repeat:
  if (!c) {
    iterator.current.type = TOKEN_END_OF_STREAM;
    return &iterator.current;
  }

  switch (c) {
    case '+': {
      advanceInputIterator();
      iterator.current.type = TOKEN_PLUS;
    } break;
    case '-': {
      c = getNextCharacter();
      if (c != '-') {
        iterator.current.type = TOKEN_MINUS;
      } else {
        while (c != '\n' && c != 0) {
          c = getNextCharacter();
        }
        goto repeat;
      }
    } break;
    case '*': {
      iterator.current.type = TOKEN_ASTERISK;
      advanceInputIterator();
    } break;
    case '/': {
      iterator.current.type = TOKEN_DIVIDE;
      c = getNextCharacter();
      if (c == '/') {
        iterator.current.type = TOKEN_DIV;
        advanceInputIterator();
      }
    } break;
    case '%': {
      iterator.current.type = TOKEN_MOD;
      advanceInputIterator();
    } break;
    case '^': {
      iterator.current.type = TOKEN_BXOR;
      advanceInputIterator();
    } break;
    case '#': {
      iterator.current.type = TOKEN_DASH;
      advanceInputIterator();
    } break;
    case '&': {
      iterator.current.type = TOKEN_AT;
      advanceInputIterator();
    } break;
    case '~': {
      iterator.current.type = TOKEN_BNOT;
      c = getNextCharacter();
      if (c == '=') {
        iterator.current.type = TOKEN_BNOT_ASSIGN;
        advanceInputIterator();
      }
    } break;
    case '|': {
      iterator.current.type = TOKEN_BOR;
      advanceInputIterator();
    } break;
    case '<': {
      iterator.current.type = TOKEN_LESS;
      c = getNextCharacter();
      if (c == '<') {
        iterator.current.type = TOKEN_BLEFT;
        advanceInputIterator();
      } else if (c == '=') {
        iterator.current.type = TOKEN_LESS_EQUAL;
        advanceInputIterator();
      }
    } break;
    case '>': {
      iterator.current.type = TOKEN_BIGGER;
      c = getNextCharacter();
      if (c == '>') {
        iterator.current.type = TOKEN_BRIGHT;
        advanceInputIterator();
      }
    } break;
    case '=': {
      iterator.current.type = TOKEN_ASSIGN;
      c = getNextCharacter();
      if (c == '=') {
        iterator.current.type = TOKEN_EQUALS;
        advanceInputIterator();
      } else if (c == '>') {
        iterator.current.type = TOKEN_BIGGER_EQUAL;
        advanceInputIterator();
      }
    } break;
    case '(': {
      iterator.current.type = TOKEN_LEFT_PAREN;
      advanceInputIterator();
    } break;
    case ')': {
      iterator.current.type = TOKEN_RIGHT_PAREN;
      advanceInputIterator();
    } break;
    case '{': {
      iterator.current.type = TOKEN_LEFT_BRACE;
      advanceInputIterator();
    } break;
    case '}': {
      iterator.current.type = TOKEN_RIGHT_BRACE;
      advanceInputIterator();
    } break;
    case '[': {
      iterator.current.type = TOKEN_LEFT_BRACKET;
      advanceInputIterator();
    } break;
    case ']': {
      iterator.current.type = TOKEN_RIGHT_BRACKET;
      advanceInputIterator();
    } break;
    case ';': {
      iterator.current.type = TOKEN_SEMICOLON;
      advanceInputIterator();
    } break;
    case ':': {
      iterator.current.type = TOKEN_COLON;
      c = getNextCharacter();
      if (c == ':') {
        iterator.current.type = TOKEN_COLON_COLON;
        advanceInputIterator();
      }
    } break;
    case ',': {
      iterator.current.type = TOKEN_COMMA;
      advanceInputIterator();
    } break;
    case '.': {
      iterator.current.type = TOKEN_PERIOD;
      c = getNextCharacter();
      if (c == '.') {
        iterator.current.type = TOKEN_2PERIOD;
        c = getNextCharacter();
        if (c == '.') {
          iterator.current.type = TOKEN_3PERIOD;
          advanceInputIterator();
        }
      }
    } break;
    case ' ':
    case '\t':
    case '\n': {
      c = getNextCharacter();
      goto repeat;
    } break;
  }
  return &iterator.current;
}
