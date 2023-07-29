#include <string.h>
#include "tokenizer.hh"

#define LUACOMP_TRIVIAL_TOKEN \
  '+' : case '*':             \
  case '%':                   \
  case '#':                   \
  case '&':                   \
  case '|':                   \
  case '(':                   \
  case ')':                   \
  case '{':                   \
  case '}':                   \
  case ']':                   \
  case ';':                   \
  case ','

#define LUACOMP_COMPOSITE_TOKEN_START \
  '/' : case '~':                     \
  case '<':                           \
  case '>':                           \
  case '=':                           \
  case ':'

#define LUACOMP_ALPHA_CHAR \
  'a' : case 'b':          \
  case 'c':                \
  case 'd':                \
  case 'e':                \
  case 'f':                \
  case 'g':                \
  case 'h':                \
  case 'i':                \
  case 'j':                \
  case 'k':                \
  case 'l':                \
  case 'm':                \
  case 'n':                \
  case 'o':                \
  case 'p':                \
  case 'q':                \
  case 'r':                \
  case 's':                \
  case 't':                \
  case 'u':                \
  case 'v':                \
  case 'w':                \
  case 'x':                \
  case 'y':                \
  case 'z':                \
  case 'A':                \
  case 'B':                \
  case 'C':                \
  case 'D':                \
  case 'E':                \
  case 'F':                \
  case 'G':                \
  case 'H':                \
  case 'I':                \
  case 'J':                \
  case 'K':                \
  case 'L':                \
  case 'M':                \
  case 'N':                \
  case 'O':                \
  case 'P':                \
  case 'Q':                \
  case 'R':                \
  case 'S':                \
  case 'T':                \
  case 'U':                \
  case 'V':                \
  case 'W':                \
  case 'X':                \
  case 'Y':                \
  case 'Z'

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

struct CharTokenPair {
  char c;
  LuaTokenType t;
};

struct ScannerTableEntry {
  LuaTokenType main_type;
  size_t size;
  CharTokenPair pairs[2];
};

// clang-format off
static const ScannerTableEntry scanner_table[] = {
    {
      .main_type = TOKEN_DIVIDE,
      .size = 1,
      .pairs = {
        {.c = '/', .t = TOKEN_DIV}
      }
    },
    {
      .main_type = TOKEN_BNOT,
      .size = 1,
      .pairs = {
        {.c = '=', .t = TOKEN_BNOT_ASSIGN}
      }
    },
    {
      .main_type = TOKEN_LESS,
      .size = 2,
      .pairs = {
        {.c = '<', .t = TOKEN_BLEFT},
        {.c = '=', .t = TOKEN_LESS_EQUAL}
      }
    },
    {
      .main_type = TOKEN_BIGGER,
      .size = 2,
      .pairs = {
        {.c = '>', .t = TOKEN_BRIGHT},
        {.c = '=', .t = TOKEN_BIGGER_EQUAL}
      }
    },
    {
      .main_type = TOKEN_ASSIGN,
      .size = 1,
      .pairs = {
        {.c = '=', .t = TOKEN_EQUALS}
      }
    },
    {
      .main_type = TOKEN_COLON,
      .size = 1,
      .pairs = {
        {.c = ':', .t = TOKEN_COLON_COLON}
      }
    }
};
// clang-format on

struct KeywordsHashTableEntry {
  const char* string;
  size_t len;
  LuaTokenType token_type;
};

#define KEYWORDS_HASH_TABLE_ENTRY(literal, type) \
  { .string = literal, .len = sizeof(literal) - 1, .token_type = type }

namespace {
inline uint32_t hashKeyword(const char* str, size_t len) {
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

#define TOTAL_KEYWORDS 22
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 8
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 29
LuaTokenType lookupKeyword(const char* str, size_t len) {
  // clang-format off
  static KeywordsHashTableEntry wordlist[] = {
    {.string = "", .len = 0, .token_type = TOKEN_END_OF_STREAM}, 
    {.string = "", .len = 0, .token_type = TOKEN_END_OF_STREAM}, 
    KEYWORDS_HASH_TABLE_ENTRY("in", TOKEN_IN),
    KEYWORDS_HASH_TABLE_ENTRY("nil", TOKEN_NIL),
    {.string = "", .len = 0, .token_type = TOKEN_END_OF_STREAM}, 
    KEYWORDS_HASH_TABLE_ENTRY("local", TOKEN_LOCAL),
    KEYWORDS_HASH_TABLE_ENTRY("return",TOKEN_RETURN),
    KEYWORDS_HASH_TABLE_ENTRY("if", TOKEN_IF),
    KEYWORDS_HASH_TABLE_ENTRY("for", TOKEN_FOR),
    {.string = "", .len = 0, .token_type = TOKEN_END_OF_STREAM}, 
    KEYWORDS_HASH_TABLE_ENTRY("while", TOKEN_WHILE),
    {.string = "", .len = 0, .token_type = TOKEN_END_OF_STREAM}, 
    KEYWORDS_HASH_TABLE_ENTRY("or", TOKEN_OR),
    KEYWORDS_HASH_TABLE_ENTRY("function", TOKEN_FUNCTION),
    KEYWORDS_HASH_TABLE_ENTRY("else", TOKEN_ELSE),
    KEYWORDS_HASH_TABLE_ENTRY("false", TOKEN_FALSE),
    KEYWORDS_HASH_TABLE_ENTRY("elseif",TOKEN_ELSEIF),
    {.string = "", .len = 0, .token_type = TOKEN_END_OF_STREAM}, 
    KEYWORDS_HASH_TABLE_ENTRY("not", TOKEN_NOT),
    KEYWORDS_HASH_TABLE_ENTRY("then", TOKEN_THEN),
    KEYWORDS_HASH_TABLE_ENTRY("until", TOKEN_UNTIL),
    KEYWORDS_HASH_TABLE_ENTRY("repeat", TOKEN_REPEAT),
    {.string = "", .len = 0, .token_type = TOKEN_END_OF_STREAM}, 
    KEYWORDS_HASH_TABLE_ENTRY("end", TOKEN_END),
    KEYWORDS_HASH_TABLE_ENTRY("true", TOKEN_TRUE),
    KEYWORDS_HASH_TABLE_ENTRY("break", TOKEN_BREAK),
    {.string = "", .len = 0, .token_type = TOKEN_END_OF_STREAM},
    KEYWORDS_HASH_TABLE_ENTRY("do", TOKEN_DO),
    KEYWORDS_HASH_TABLE_ENTRY("and", TOKEN_AND),
    KEYWORDS_HASH_TABLE_ENTRY("goto", TOKEN_GOTO)
  };
  // clang-format on

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
    uint8_t key = hashKeyword(str, len);

    if (key <= MAX_HASH_VALUE) {
      const KeywordsHashTableEntry* s = &wordlist[key];

      if (len == s->len && !strncmp(str, s->string, len)) return s->token_type;
    }
  }
  return TOKEN_END_OF_STREAM;
}
}  // namespace

TokenIterator TokenIterator::New(
    const char* input_name, const char* input, usize size) {
  return TokenIterator{
      .input_ = input, .input_name_ = input_name, .input_size_ = size};
}

TokenIterator::operator bool() const {
  return current_token_.type != TOKEN_END_OF_STREAM;
}

TokenIterator& TokenIterator::operator++() {
  nextToken();
  return *this;
}

const Token& TokenIterator::operator*() const {
  return current_token_;
}
char TokenIterator::nextCharacter() {
  return input_[++current_input_pos_];
}

char TokenIterator::peekCharacter() {
  return input_[current_input_pos_];
}

void TokenIterator::scanWithTable() {
  size_t table_index = 0;
  switch (peekCharacter()) {
    case '/': table_index = 0; break;
    case '~': table_index = 1; break;
    case '<': table_index = 2; break;
    case '>': table_index = 3; break;
    case '=': table_index = 4; break;
    case ';': table_index = 5; break;
  }

  const auto& entry = scanner_table[table_index];
  char c = nextCharacter();
  for (size_t i = 0; i < entry.size; ++i) {
    if (c == entry.pairs[i].c) {
      current_token_.type = entry.pairs[i].t;
      return;
    }
  }
  current_token_.type = static_cast<LuaTokenType>(c);
}

void TokenIterator::unexpectedCharacter() {
  Panic("Unexpected token '%c' at %s:%llu:%llu\n", peekCharacter(),
      input_name_ ? input_name_ : "", line_number_, current_input_pos_ + 1);
}

void TokenIterator::nextToken() {
scanForNextToken_Label_Repeat:
  if (current_input_pos_ == input_size_) {
    current_token_.type = TOKEN_END_OF_STREAM;
    return;
  }

  char c = input_[current_input_pos_];

  switch (c) {
    case LUACOMP_TRIVIAL_TOKEN: {
      current_token_.type = static_cast<LuaTokenType>(c);
      ++current_input_pos_;
    } break;
    case LUACOMP_COMPOSITE_TOKEN_START: {
      scanWithTable();
      ++current_input_pos_;
    } break;
    case LUACOMP_ALPHA_CHAR: {
      auto start = current_input_pos_;
      char c = nextCharacter();
      while (isAlpha(c) || isDigit(c) || c == '_') {
        c = nextCharacter();
      }
      const usize len = current_input_pos_ - start;

      LuaTokenType token_type = lookupKeyword(input_ + start, len);
      if (token_type != TOKEN_END_OF_STREAM) {
        current_token_.type = token_type;
      } else {
        current_token_.type = TOKEN_IDENTIFIER;
      }
    } break;
    case '\n': {
      ++line_number_;
      ++current_input_pos_;
    } break;
    case ' ':
    case '\r':
    case '\t': {
      while (input_[++current_input_pos_] == c &&
             current_input_pos_ != input_size_) {
      }
      goto scanForNextToken_Label_Repeat;
    } break;
    default: {
      unexpectedCharacter();
    } break;
  }
}
