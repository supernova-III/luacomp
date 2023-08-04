#include "tokenizer.hh"
#include "lib.hh"

#define VAR(str, tok) \
  { str, sizeof(str) - 1, tok }
#define KWTABLE_ENTRY3(c, v0, t0, v1, t1, v2, t2) \
  trie_[idx(#@ c)] = {                            \
      .size = 3, .variants = {VAR(v0, t0), VAR(v1, t1), VAR(v2, t2)}}

#define KWTABLE_ENTRY2(c, v0, t0, v1, t1) \
  trie_[idx(#@ c)] = {.size = 2, .variants = {VAR(v0, t0), VAR(v1, t1)}}

#define KWTABLE_ENTRY(c, v, t) \
  trie_[idx(#@ c)] = {.size = 1, .variants = {VAR(v, t)}}

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

#define LUACOMP_DIGIT_CHAR '0': case '1': case '2': case '3': case '4': case '5': \
  case '6': case '7': case '8': case '9'
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
      return c - 'a' + 10;
    }
    return c - 'A' + 10;
  }
  return charToDigit(c);
}

inline constexpr size_t idx(char c) {
  return c - 'a';
}

class KeywordsTable {
  struct TableEntry {
    struct Tok {
      const char* str;
      size_t len;
      LuaTokenType token;
    };
    size_t size = 0;
    Tok variants[3] = {};
  };

  TableEntry trie_[idx('z') + 1] = {};

 public:
  consteval KeywordsTable() {
    KWTABLE_ENTRY(a, "and", TOKEN_AND);
    KWTABLE_ENTRY(b, "break", TOKEN_BREAK);
    KWTABLE_ENTRY(d, "do", TOKEN_DO);
    KWTABLE_ENTRY3(
        e, "else", TOKEN_ELSE, "elseif", TOKEN_ELSEIF, "end", TOKEN_END);
    KWTABLE_ENTRY3(
        f, "false", TOKEN_FALSE, "for", TOKEN_FOR, "function", TOKEN_FUNCTION);
    KWTABLE_ENTRY(g, "goto", TOKEN_GOTO);
    KWTABLE_ENTRY2(i, "if", TOKEN_IF, "in", TOKEN_IN);
    KWTABLE_ENTRY(l, "local", TOKEN_LOCAL);
    KWTABLE_ENTRY2(n, "nil", TOKEN_NIL, "not", TOKEN_NOT);
    KWTABLE_ENTRY(o, "or", TOKEN_OR);
    KWTABLE_ENTRY2(r, "repeat", TOKEN_REPEAT, "return", TOKEN_RETURN);
    KWTABLE_ENTRY2(t, "then", TOKEN_THEN, "true", TOKEN_TRUE);
    KWTABLE_ENTRY(u, "until", TOKEN_UNTIL);
    KWTABLE_ENTRY(w, "while", TOKEN_WHILE);
  }

  const auto& operator[](char c) const { return trie_[idx(c)]; }
};

#undef VAR
#undef KWTABLE_ENTRY
#undef KWTABLE_ENTRY2
#undef KWTABLE_ENTRY3

LuaTokenType recognizeKeywordsWithTable(const char* str, size_t len) {
  static constinit auto trie = KeywordsTable();
  const auto& entry = trie[*str];
  for (size_t i = 0; i < entry.size; ++i) {
    const auto& [tok_str, tok_len, tok] = entry.variants[i];
    if (tok_len == len && !strncmp(tok_str, str, len)) {
      return tok;
    }
  }
  return TOKEN_IDENTIFIER;
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

void TokenIterator::recognizeTokensWithTable() {
  struct CharTokenPair {
    char c;
    LuaTokenType t;
  };
  struct ScannerTableEntry {
    LuaTokenType main_type;
    size_t size;
    CharTokenPair pairs[2];
  };
#define STENTRY0(mt) \
  { .main_type = mt }
#define STENTRY1(mt, c0, t0)                                    \
  {                                                             \
    .main_type = mt, .size = 1, .pairs = { {.c = c0, .t = t0} } \
  }
#define STENTRY2(mt, c1, t1, c2, t2)       \
  {                                        \
    .main_type = mt, .size = 2, .pairs = { \
      {.c = c1, .t = t1},                  \
      {.c = c2, .t = t2}                   \
    }                                      \
  }
  static constexpr ScannerTableEntry scanner_table[] = {
      STENTRY1(TOKEN_DIVIDE, '/', TOKEN_DIV),
      STENTRY1(TOKEN_BNOT, '=', TOKEN_BNOT_ASSIGN),
      STENTRY2(TOKEN_LESS, '<', TOKEN_BLEFT, '=', TOKEN_LESS_EQUAL),
      STENTRY2(TOKEN_BIGGER, '>', TOKEN_BRIGHT, '=', TOKEN_BIGGER_EQUAL),
      STENTRY1(TOKEN_ASSIGN, '=', TOKEN_EQUALS),
      STENTRY1(TOKEN_COLON, ':', TOKEN_COLON_COLON), STENTRY0(TOKEN_PLUS),
      STENTRY1(TOKEN_MINUS, '-', TOKEN_COMMENT), STENTRY0(TOKEN_ASTERISK),
      STENTRY0(TOKEN_MOD), STENTRY0(TOKEN_BXOR), STENTRY0(TOKEN_DASH),
      STENTRY0(TOKEN_AT), STENTRY0(TOKEN_BOR), STENTRY0(TOKEN_LEFT_PAREN),
      STENTRY0(TOKEN_RIGHT_PAREN), STENTRY0(TOKEN_LEFT_BRACE),
      STENTRY0(TOKEN_RIGHT_BRACE), STENTRY0(TOKEN_LEFT_BRACKET),
      STENTRY0(TOKEN_RIGHT_BRACKET), STENTRY0(TOKEN_SEMICOLON),
      STENTRY0(TOKEN_COMMA)};

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
  throw RuntimeError("Unexpected token '%c' at %s:%llu:%llu", peekCharacter(),
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

struct ScanIntegerResult {
  double magnitude;
  double power_of_base;
};

static inline ScanIntegerResult scanInteger(
    const char* str, size_t len, double base) {
  auto transform = base == 10 ? charToDigit : hexToNumber;
  double magnitude = 0;
  double power_of_base = 1;
  for (size_t i = 0; i < len; ++i) {
    const auto index = len - i - 1;
    const auto c = str[index];
    magnitude += power_of_base * transform(c);
    power_of_base *= base;
  }
  return {magnitude, power_of_base};
}

double TokenIterator::recognizeExponent(
    CheckingFunction checker, double exp_base) {
  double result = 1;
  char c = nextCharacter();
  double sign = 1;
  if (c == '-') {
    sign = -1;
    nextCharacter();
  } else if (c == '+') {
    c = nextCharacter();
  }
  const auto [str, len] = scanString(checker);
  const auto [number, _] = scanInteger(str, len, 10);
  auto power = static_cast<size_t>(number);
  double base_in_power = 1;
  for (size_t i = 0; i < power; ++i) {
    base_in_power *= exp_base;
  }
  if (sign > 0) {
    result *= base_in_power;
  } else {
    result /= base_in_power;
  }
  return result;
}

void TokenIterator::nextToken() {
  while (true) {
    if (current_input_pos_ >= input_size_ || peekCharacter() == '\0') {
      current_token_.type = TOKEN_END_OF_STREAM;
      return;
    }

    char c = input_[current_input_pos_];

    switch (c) {
      case '.': {
        const char* start = input_;
        current_token_.type = TOKEN_PERIOD;
        c = nextCharacter();
        if (c == '.') {
          current_token_.type = TOKEN_2PERIOD;
          c = nextCharacter();
          if (c == '.') {
            current_token_.type = TOKEN_3PERIOD;
            ++current_input_pos_;
          } else {
            return;
          }
        }

        /*
        if (isDigit(c)) {
          scanString(isDigit);
          c = peekCharacter();
          if (c == 'E' || c == 'e') {
            c = nextCharacter();
            if (c == '+' || c == '-') {
              c = nextCharacter();
            }
            if (isDigit(c)) {
              scanString(isDigit);
            } else {
              unexpectedCharacter();
            }
          }
          const size_t number_string_len = input_ + current_input_pos_ - start;
          current_token_.type = TOKEN_NUMBER;
          current_token_.value.number =
              EvaluateNumber(start, number_string_len);
        }
        */

        if (isDigit(c)) {
          const auto [str, len] = scanString(isDigit);
          const auto [magnitude, power_of_ten] = scanInteger(str, len, 10);
          double result = magnitude / power_of_ten;
          c = peekCharacter();
          if (c == 'E' || c == 'e') {
            result *= recognizeExponent(isDigit, 10);
          }
          current_token_.type = TOKEN_NUMBER;
          current_token_.value.number = result;
        }
        return;
      }
      case LUACOMP_DIGIT_CHAR: {
        current_token_.type = TOKEN_NUMBER;
        double base = 10;
        if (c == '0') {
          c = nextCharacter();
          if (current_input_pos_ == input_size_) {
            current_token_.value.number = 0;
            return;
          }
          if (c == 'x' || c == 'X') {
            base = 16;
            c = nextCharacter();
          }
        }

        // Here the current char position points to the character right after 0x
        // (0X) in case of base = 16, otherwise the leading zero is just skipped
        // and it has no effect on the result

        auto checker = base == 10 ? isDigit : isHexadecimal;
        const auto [str, len] = scanString(checker);
        const auto [number, _] = scanInteger(str, len, base);
        double result = number;
        c = peekCharacter();
        if (c == '.') {
          c = nextCharacter();
          if (checker(c)) {
            const auto [str, len] = scanString(checker);
            const auto [number, magnitude] = scanInteger(str, len, base);
            result += number / magnitude;
          }
        }
        c = peekCharacter();
        if (c == 'E' || c == 'e' || c == 'p' || c == 'P') {
          double exp_base = 10;
          if (c == 'p' || c == 'P') {
            if (base == 10) unexpectedCharacter();
            exp_base = 2;
          }
          result *= recognizeExponent(checker, exp_base);
        }

        current_token_.value.number = result;

        return;
      }
      case LUACOMP_SPECIAL_CHAR: {
        recognizeTokensWithTable();
        return;
      }
      case LUACOMP_ALPHA_CHAR: {
        const auto [str, len] = scanString(isKeywordCharacter);
        LuaTokenType token_type = recognizeKeywordsWithTable(str, len);
        current_token_.type = token_type;
        if (token_type == TOKEN_IDENTIFIER) {
          string_table_.insert(std::string(str, len));
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
               current_input_pos_ < input_size_) {
        }
      } break;
      default: {
        unexpectedCharacter();
      }
    }
  }
}

struct EvaluateIntegerResult {
  double magnitude;
  double power;
};

EvaluateIntegerResult evaluateIntegerFromString(const char* str, size_t len,
    NumberBase base, TransformingFunction transform) {
  double magnitude = 0;
  double power_of_base = 1;
  for (size_t i = 0; i < len; ++i) {
    const size_t index = len - i - 1;
    magnitude += power_of_base * transform(str[index]);
    power_of_base *= base;
  }
  return {magnitude, power_of_base};
}

// dot_position == 0 => .123
// dot_position == len - 1 => 123., same as 123, no exponent
// dot_position == len => no dot
// dot_position > 0 && dot_position < len - 1 => 123.123
// exponent_position == len => no exponent
// TODO: it's better to create a class to control this state and simplify the
// function usage
double EvaluateNumber(const char* string, size_t len, NumberBase base,
    TransformingFunction transform, size_t dot_position,
    ExponentType exponent_type, size_t exponent_position) {
  const auto [integer_part, _] =
      evaluateIntegerFromString(string, dot_position, base, transform);
  if (dot_position == len - 1 || dot_position == len) {
    return integer_part;
  }

  // 123.123e12
  // dot_pos = 3
  // exponent_pos = 7
  // len(123) = exponent_pos - dot_pos - 1
  const auto fractional_part_len = exponent_position - dot_position - 1;
  double fractional_part = 0;
  const auto [res, power] = evaluateIntegerFromString(
      string + dot_position + 1, fractional_part_len, base, transform);
  fractional_part = res / power;

  size_t next_pos = 0;
  double sign = 1;
  switch (exponent_type) {
    case ExponentType::NONE: return integer_part + fractional_part;
    case ExponentType::PLAIN: next_pos = exponent_position + 1; break;
    case ExponentType::MINUS:
    case ExponentType::PLUS: {
      next_pos = exponent_position + 2;
      sign -= 2 * (exponent_type == ExponentType::MINUS);
    }
  }

  const size_t exponent_number_len = len - next_pos;
  const auto [exponent_number, __] = evaluateIntegerFromString(
      string + next_pos, exponent_number_len, base, transform);
  double exponent_part = 1;
  for (size_t i = 0; i < exponent_number_len; ++i) {
    exponent_part *= base;
  }

  return (integer_part + fractional_part) * exponent_part;
}

ScanNumberError ScanNumber(
    const char* string, size_t len, size_t& matched_substring_len) {
  const char* cur = string;
  auto exhausted = [&](const char* cur) { return cur - string >= len; };

  auto base = NumberBase(NumberBase::Enum::DEC);

  // Matching 0x or 0X
  if (*cur == '0') {
    ++cur;
    if (!exhausted(cur) && (*cur == 'x' || *cur == 'X')) {
      if (len < 3) {
        return ScanNumberError::UNEXPECTED_END;
      }
      base.val = NumberBase::Enum::HEX;
      ++cur;
    }
  }

  CheckingFunction checker = isDigit;
  CheckingFunction exponent_checker = [](char c) {
    return c == 'e' || c == 'E';
  };
  if (base.val == NumberBase::Enum::HEX) {
    checker = isHexadecimal;
    exponent_checker = [](char c) { return c == 'p' || c == 'P'; };
  }

  size_t integer_part_len = 0;
  while (!exhausted(cur) && checker(*cur)) {
    ++cur;
    ++integer_part_len;
  }

  if (*cur == '.') {
    ++cur;
  }

  if (exhausted(cur)) {
    if (integer_part_len == 0) {
      return ScanNumberError::UNEXPECTED_END;
    }
    matched_substring_len = cur - string;
    return {};
  }

  size_t fractional_part_len = 0;
  while (!exhausted(cur) && checker(*cur)) {
    ++cur;
    ++fractional_part_len;
  }

  if (fractional_part_len + integer_part_len == 0) {
    return ScanNumberError::NO_NUMBER;
  }

  if (exponent_checker(*cur)) {
    ++cur;
    if (!exhausted(cur)) {
      if (*cur == '+' || *cur == '-') {
        ++cur;
        if (exhausted(cur)) {
          return ScanNumberError::UNEXPECTED_END;
        }
      }
      if (checker(*cur)) {
        while (!exhausted(cur) && isDigit(*cur)) {
          ++cur;
        }
      } else {
        return ScanNumberError::UNEXPECTED_END;
      }
    }
  }
  matched_substring_len = cur - string;
  return {};
}
