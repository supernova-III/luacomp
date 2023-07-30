#include <string.h>
#include <stdlib.h>
#include "tokenizer.hh"
#include <format>

// clang-format off
#define LUACOMP_TRIVIAL_TOKEN \
  '+' : case '*': case '%': case '#': case '&': case '|': \
  case '(': case ')': case '{': case '}': case ']': \
  case ';': case ','

#define LUACOMP_COMPOSITE_TOKEN_START \
  '/' : case '~': case '<': case '>': \
  case '=': case ':'

#define LUACOMP_ALPHA_CHAR 'a' : case 'b': case 'c': case 'd': case 'e': case 'f':\
  case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':\
  case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':\
  case 's': case 't': case 'u': case 'v': case 'w': case 'x':\
  case 'y': case 'z': case 'A': case 'B': case 'C': case 'D':\
  case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':\
  case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':\
  case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':\
  case 'W': case 'X': case 'Y': case 'Z'
// clang-format on

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

LuaTokenType lookupKeyword(const char* str, size_t len) {
  struct KeywordsTableEntry {
    std::string_view string;
    LuaTokenType token_type;
  };

  // clang-format off
  static constexpr KeywordsTableEntry kw_table[] = {
    {"and", TOKEN_AND}, {"break", TOKEN_BREAK}, 
    {"do", TOKEN_DO}, {"else", TOKEN_ELSE},
    {"elseif", TOKEN_ELSEIF}, {"end", TOKEN_END}, 
    {"false", TOKEN_FALSE}, {"for", TOKEN_FOR}, 
    {"function", TOKEN_FUNCTION}, {"goto", TOKEN_GOTO},
    {"if", TOKEN_IF}, {"in", TOKEN_IN}, 
    {"local", TOKEN_LOCAL},{"nil", TOKEN_NIL}, 
    {"not", TOKEN_NOT}, {"or", TOKEN_OR},
    {"repeat", TOKEN_REPEAT}, {"return", TOKEN_RETURN}, 
    {"then", TOKEN_THEN}, {"true", TOKEN_TRUE}, 
    {"until", TOKEN_UNTIL}, {"while", TOKEN_WHILE}
  };
  // clang-format on
  for (const auto& [s, t] : kw_table) {
    if (s == std::string_view(str, len)) {
      return t;
    }
  }
  return TOKEN_END_OF_STREAM;
}
}  // namespace

TokenIterator::TokenIterator(
    const char* input_name, const char* input, usize size)
    : input_(input), input_name_(input_name), input_size_(size) {}

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
  static constexpr ScannerTableEntry scanner_table[] = {
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
  const auto message =
      std::format("Unexpected token '{}' at {}:{}:{}", peekCharacter(),
          input_name_ ? input_name_ : "", line_number_, current_input_pos_ + 1);
  throw std::runtime_error(message);
}

TokenIterator::ScanStringResult TokenIterator::scanString(CheckingFunction f) {
  const auto start = current_input_pos_;
  while (f(peekCharacter())) {
    nextCharacter();
  }
  const auto len = current_input_pos_ - start;
  return {.str = input_ + start, .len = len};
}

void TokenIterator::nextToken() {
scanForNextToken_Label_Repeat:
  if (current_input_pos_ == input_size_) {
    current_token_.type = TOKEN_END_OF_STREAM;
    return;
  }

  char c = input_[current_input_pos_];

  switch (c) {
    case '.': {
      current_token_.type = TOKEN_PERIOD;
      c = nextCharacter();
      if (c == '.') {
        current_token_.type = TOKEN_2PERIOD;
        c = nextCharacter();
        if (c == '.') {
          current_token_.type = TOKEN_3PERIOD;
          ++current_input_pos_;
        }
      }
      /*
      if (isDigit(c)) {
        current_token_.type = TOKEN_NUMBER;
        f64 result = 0;

        const auto start = current_input_pos_;
        while (isDigit(c)) {
          c = nextCharacter();
        }
        const auto len = current_input_pos_ - start;
      }
      */
    } break;
    case LUACOMP_TRIVIAL_TOKEN: {
      current_token_.type = static_cast<LuaTokenType>(c);
      ++current_input_pos_;
    } break;
    case LUACOMP_COMPOSITE_TOKEN_START: {
      scanWithTable();
      ++current_input_pos_;
    } break;
    case LUACOMP_ALPHA_CHAR: {
      const auto [str, len] = scanString(isKeywordCharacter);
      LuaTokenType token_type = lookupKeyword(str, len);
      if (token_type != TOKEN_END_OF_STREAM) {
        current_token_.type = token_type;
      } else {
        string_table_.insert(std::string(str, len));
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
