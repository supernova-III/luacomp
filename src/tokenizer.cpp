#include "tokenizer.hh"
#include "lib.hh"

// clang-format off
#define LUACOMP_SPECIAL_CHAR\
  '+' : case '*': case '%': case '#': case '&': case '|': \
  case '(': case ')': case '{': case '}': case ']': \
  case ';': case ',':\
  case '/' : case '~': case '<': case '>': \
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

class KeywordsTrie {
  struct TableEntry {
    struct Tok {
      const char* str;
      size_t len;
      LuaTokenType token;
    };
    size_t size = 0;
    Tok variants[3] = {};
  };
  struct TokStr {
    const char* str;
    size_t len;
  };

  TableEntry trie_['z' - 'a'] = {};

 public:
  consteval KeywordsTrie() {
#define VAR(str, tok) \
  { str, sizeof(str) - 1, tok }
#define idx(c) c - 'a'
    // clang-format off
    trie_[idx('a')] = {.size = 1, .variants = {VAR("and", TOKEN_AND)}};
    trie_[idx('b')] = {.size = 1, .variants = {VAR("break", TOKEN_BREAK)}};
    trie_[idx('d')] = {.size = 1, .variants = {VAR("do", TOKEN_DO)}};
    trie_[idx('e')] = {
      .size = 3,
      .variants = {
        VAR("else", TOKEN_ELSE), 
        VAR("elseif", TOKEN_ELSEIF),
        VAR("end", TOKEN_END)
      }
    };
    trie_[idx('f')] = {
      .size = 3,
      .variants = {
        VAR("false", TOKEN_FALSE), 
        VAR("for", TOKEN_FOR),
        VAR("function", TOKEN_FUNCTION)
      }
    };
    trie_[idx('g')] = {.size = 1, .variants = {VAR("goto", TOKEN_GOTO)}};
    trie_[idx('i')] = {
      .size = 2, 
      .variants = {
        VAR("if", TOKEN_IF), 
        VAR("in", TOKEN_IN)
      }
    };
    trie_[idx('l')] = {.size = 1, .variants = {VAR("local", TOKEN_LOCAL)}};
    trie_[idx('n')] = {
      .size = 2, 
      .variants = {
        VAR("nil", TOKEN_NIL), 
        VAR("not", TOKEN_NOT)
      }
    };
    trie_[idx('o')] = {.size = 1, .variants = {VAR("or", TOKEN_OR)}};
    trie_[idx('r')] = {
      .size = 2,
      .variants = {
        VAR("repeat", TOKEN_REPEAT), 
        VAR("return", TOKEN_RETURN)
      }
    };
    trie_[idx('t')] = {
      .size = 2,
      .variants = {
        VAR("then", TOKEN_THEN), 
        VAR("true", TOKEN_TRUE)
      }
    };
    trie_[idx('u')] = {.size = 1, .variants = {VAR("until", TOKEN_UNTIL)}};
    trie_[idx('w')] = {.size = 1, .variants = {VAR("while", TOKEN_WHILE)}};
    // clang-format on
#undef VAR
  }

  const auto& operator[](char c) const { return trie_[c - 'a']; }
};

LuaTokenType lookupKeyword(const char* str, size_t len) {
  static constinit auto trie = KeywordsTrie();
  const auto& entry = trie[*str];
  for (size_t i = 0; i < entry.size; ++i) {
    const auto& [tok_str, tok_len, tok] = entry.variants[i];
    if (tok_len == len && !strncmp(tok_str, str, len)) {
      return tok;
    }
  }
  return TOKEN_END_OF_STREAM;
}
}  // namespace

TokenIterator::TokenIterator(
    const char* input_name, const char* input, size_t size)
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
      }, 
      {
        .main_type = TOKEN_PLUS,
      },
      {
        .main_type = TOKEN_MINUS,
        .size = 1,
        .pairs = {
          { .c = '-', .t = TOKEN_COMMENT }
        }
      },
      {
        .main_type = TOKEN_ASTERISK,
      },
      {
        .main_type = TOKEN_MOD
      },
      {
        .main_type = TOKEN_BXOR
      },
      {
        .main_type = TOKEN_DASH
      },
      {
        .main_type = TOKEN_AT
      },
      {
        .main_type = TOKEN_BOR
      },
      {
        .main_type = TOKEN_LEFT_PAREN
      },
      {
        .main_type = TOKEN_RIGHT_PAREN
      },
      {
        .main_type = TOKEN_LEFT_BRACE
      },
      {
        .main_type = TOKEN_RIGHT_BRACE
      },
      {
        .main_type = TOKEN_LEFT_BRACKET
      },
      {
        .main_type = TOKEN_RIGHT_BRACKET
      },
      {
        .main_type = TOKEN_SEMICOLON
      },
      {
        .main_type = TOKEN_COMMA
      },
  };
  // clang-format on

  size_t table_index = TOKEN_DIVIDE;
  switch (peekCharacter()) {
    case '~': table_index = TOKEN_BNOT; break;
    case '<': table_index = TOKEN_LESS; break;
    case '>': table_index = TOKEN_BIGGER; break;
    case '=': table_index = TOKEN_ASSIGN; break;
    case ':': table_index = TOKEN_COLON; break;
    case '+': table_index = TOKEN_PLUS; break;
    case '-': table_index = TOKEN_MINUS; break;
    case '*': table_index = TOKEN_ASTERISK; break;
    case '%': table_index = TOKEN_MOD; break;
    case '^': table_index = TOKEN_BXOR; break;
    case '#': table_index = TOKEN_DASH; break;
    case '&': table_index = TOKEN_AT; break;
    case '|': table_index = TOKEN_BOR; break;
    case '(': table_index = TOKEN_LEFT_PAREN; break;
    case ')': table_index = TOKEN_RIGHT_PAREN; break;
    case '{': table_index = TOKEN_LEFT_BRACE; break;
    case '}': table_index = TOKEN_RIGHT_BRACE; break;
    case '[': table_index = TOKEN_LEFT_BRACKET; break;
    case ']': table_index = TOKEN_RIGHT_BRACKET; break;
    case ';': table_index = TOKEN_SEMICOLON; break;
    case ',': table_index = TOKEN_COMMA; break;
  }

  const auto& entry = scanner_table[table_index];
  char c = nextCharacter();
  for (size_t i = 0; i < entry.size; ++i) {
    if (c == entry.pairs[i].c) {
      current_token_.type = entry.pairs[i].t;
      ++current_input_pos_;
      return;
    }
  }
  current_token_.type = static_cast<LuaTokenType>(entry.main_type);
}

void TokenIterator::unexpectedCharacter() {
  throw RuntimeError("Unexpected token '%c' at %llu:%llu:%llu", peekCharacter(),
      input_name_ ? input_name_ : "", line_number_, current_input_pos_ + 1);
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
  while (true) {
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
        return;
      }
      case LUACOMP_SPECIAL_CHAR: {
        scanWithTable();
        return;
      }
      case LUACOMP_ALPHA_CHAR: {
        const auto [str, len] = scanString(isKeywordCharacter);
        LuaTokenType token_type = lookupKeyword(str, len);
        if (token_type != TOKEN_END_OF_STREAM) {
          current_token_.type = token_type;
        } else {
          string_table_.insert(std::string(str, len));
          current_token_.type = TOKEN_IDENTIFIER;
        }
        return;
      }
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
      } break;
      default: {
        unexpectedCharacter();
      }
    }
  }
}
