#include "tokenizer.hh"
#include "lib.hh"

#define VAR(str, tok) \
  { String{str, sizeof(str) - 1}, tok }
#define KWTABLE_ENTRY3(c, v0, t0, v1, t1, v2, t2) \
  trie_[idx(#@ c)] = {                            \
      .size = 3, .variants = {VAR(v0, t0), VAR(v1, t1), VAR(v2, t2)}}

#define KWTABLE_ENTRY2(c, v0, t0, v1, t1) \
  trie_[idx(#@ c)] = {.size = 2, .variants = {VAR(v0, t0), VAR(v1, t1)}}

#define KWTABLE_ENTRY(c, v, t) \
  trie_[idx(#@ c)] = {.size = 1, .variants = {VAR(v, t)}}

// clang-format off
#define LUACOMP_TOKEN\
  '+' : case '*': case '%': case '#': case '&': case '|': \
  case '(': case ')': case '{': case '}': case ']': \
  case ';': case ',':\
  case '/' : case '~': case '<': case '>': \
  case '=': case ':': case '-'

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

// Represents base of a number
struct NumberBase {
  enum struct Enum : uint8_t {
    DEC = 10,
    HEX = 16
  };

  static consteval NumberBase Hex() { return Enum::HEX; }
  static consteval NumberBase Dec() { return Enum::DEC; }

  constexpr NumberBase(Enum v) : val(v) {}

  Enum val;

  operator double() const { return static_cast<double>(val); }

  NumberBase::Enum operator*() const { return val; }
};

namespace {
bool isHexChar(char c) {
  return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

double charToDigit(char c) {
  return c - '0';
}

double hexToNumber(char c) {
  if (isHexChar(c)) {
    if (c >= 'a') {
      return c - 'a' + 10;
    }
    return c - 'A' + 10;
  }
  return charToDigit(c);
}

bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isHexadecimal(char c) {
  return isDigit(c) || isHexChar(c);
}

bool isKeywordCharacter(char c) {
  return isAlpha(c) || isDigit(c) || c == '_';
}

struct EvaluateIntegerResult {
  double magnitude;
  double power;
};

EvaluateIntegerResult evaluateIntegerFromString(const char *str, size_t len,
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
}  // namespace

EvaluateNumberResult TryEvaluateNumber(const char *string, size_t len) {
  const char *cur = string;
  auto exhausted = [&](const char *cur) { return cur - string >= len; };
  auto base = NumberBase::Dec();
  // Matching 0x or 0X
  if (*cur == '0') {
    ++cur;
    if (!exhausted(cur) && (*cur == 'x' || *cur == 'X')) {
      if (len < 3) {
        return ScanNumberError::UNEXPECTED_END;
      }
      base = NumberBase::Hex();
      ++cur;
    }
  }

  CheckingFunction checker = isDigit;
  CheckingFunction exponent_checker = [](char c) {
    return c == 'e' || c == 'E';
  };
  TransformingFunction transform = charToDigit;
  if (base == NumberBase::Hex()) {
    checker = isHexadecimal;
    exponent_checker = [](char c) { return c == 'p' || c == 'P'; };
    transform = hexToNumber;
  }

  size_t integer_part_len = 0;
  const char *integer_part_str = cur;
  while (!exhausted(cur) && checker(*cur)) {
    ++cur;
    ++integer_part_len;
  }

  const auto integer_part_result = evaluateIntegerFromString(
      integer_part_str, integer_part_len, base, transform);
  const double integer_part = integer_part_result.magnitude;

  if (*cur == '.') {
    ++cur;
  }

  if (exhausted(cur)) {
    if (integer_part_len == 0) {
      return ScanNumberError::UNEXPECTED_END;
    }
    return EvaluateNumberResult(integer_part, cur - string);
  }

  size_t fractional_part_len = 0;
  const char *fractional_part_str = cur;
  while (!exhausted(cur) && checker(*cur)) {
    ++cur;
    ++fractional_part_len;
  }

  if (fractional_part_len + integer_part_len == 0) {
    return ScanNumberError::NO_NUMBER;
  }

  const auto fractional_part_result = evaluateIntegerFromString(
      fractional_part_str, fractional_part_len, base, transform);
  const double fractional_part =
      fractional_part_result.magnitude / fractional_part_result.power;
  double exponent_part = 1;

  if (exponent_checker(*cur)) {
    const size_t exponent_position = cur - string;
    char exponent_sign = 1;
    ++cur;
    if (!exhausted(cur)) {
      if (*cur == '+' || *cur == '-') {
        if (*cur == '-') {
          exponent_sign = -1;
        }
        ++cur;
        if (exhausted(cur)) {
          return ScanNumberError::UNEXPECTED_END;
        }
      }
      const char *exponent_number_str = cur;
      if (checker(*cur)) {
        while (!exhausted(cur) && isDigit(*cur)) {
          ++cur;
        }
      } else {
        return ScanNumberError::UNEXPECTED_END;
      }
      size_t exponent_number_len = cur - exponent_number_str;
      if (exponent_number_len == 0) {
        return ScanNumberError::UNEXPECTED_END;
      }
      const double exponent_number =
          evaluateIntegerFromString(exponent_number_str, exponent_number_len,
              NumberBase::Dec(), charToDigit)
              .magnitude;
      const double exponent_base = 10 - 8 * (base == NumberBase::Hex());
      for (size_t i = 0; i < static_cast<size_t>(exponent_number); ++i) {
        exponent_part *= exponent_base;
      }
      if (exponent_sign == -1) {
        exponent_part = 1 / exponent_part;
      }
    }
  }
  const auto number = (integer_part + fractional_part) * exponent_part;
  return EvaluateNumberResult(number, cur - string);
}
namespace {

inline constexpr size_t idx(char c) {
  return c - 'a';
}

// Keywords table that powers table-driven scanning of keywords. Implements a
// trie. In this case this is actually a hash table with predefined number of
// collisions and extremely cheap hash function that calculates a hash in a
// single op. So this is also near to ideal hash table
class KeywordsTable {
  // The table consist of list of entries. Each entry is a list of possible
  // keywords starting with specific letter
  struct TableEntry {
    struct Tok {
      String string;
      LuaTokenType token;
    };
    size_t size = 0;
    Tok variants[3] = {};
  };

  TableEntry trie_[idx('z') + 1] = {};

 public:
  consteval KeywordsTable() {
    // clang-format off
    KWTABLE_ENTRY (a, "and", TOKEN_AND);
    KWTABLE_ENTRY (b, "break", TOKEN_BREAK);
    KWTABLE_ENTRY (d, "do", TOKEN_DO);
    KWTABLE_ENTRY3(e, "else", TOKEN_ELSE, "elseif", TOKEN_ELSEIF, "end", TOKEN_END);
    KWTABLE_ENTRY3(f, "false", TOKEN_FALSE, "for", TOKEN_FOR, "function", TOKEN_FUNCTION);
    KWTABLE_ENTRY (g, "goto", TOKEN_GOTO);
    KWTABLE_ENTRY2(i, "if", TOKEN_IF, "in", TOKEN_IN);
    KWTABLE_ENTRY (l, "local", TOKEN_LOCAL);
    KWTABLE_ENTRY2(n, "nil", TOKEN_NIL, "not", TOKEN_NOT);
    KWTABLE_ENTRY (o, "or", TOKEN_OR);
    KWTABLE_ENTRY2(r, "repeat", TOKEN_REPEAT, "return", TOKEN_RETURN);
    KWTABLE_ENTRY2(t, "then", TOKEN_THEN, "true", TOKEN_TRUE);
    KWTABLE_ENTRY (u, "until", TOKEN_UNTIL);
    KWTABLE_ENTRY (w, "while", TOKEN_WHILE);
    // clang-format on
  }

  // Looks up for a keyword. It looks just like a simple hash table lookup.
  LuaTokenType operator[](const String &string) const noexcept;
};

LuaTokenType KeywordsTable::operator[](const String &string) const noexcept {
  const TableEntry &entry = trie_[idx(string[0])];
  for (size_t i = 0; i < entry.size; ++i) {
    const auto &[variant_string, tok] = entry.variants[i];
    if (variant_string == string) {
      return tok;
    }
  }
  return TOKEN_IDENTIFIER;
}
}  // namespace

TokenIterator::TokenIterator(
    const char *input_name, const char *input, size_t size)
    : input_(input),
      input_name_(input_name),
      input_size_(size),
      string_table_(128) {}

TokenIterator::operator bool() const {
  return current_token_.type != TOKEN_END_OF_STREAM;
}

TokenIterator &TokenIterator::operator++() {
  nextToken();
  return *this;
}

const Token &TokenIterator::operator*() const {
  return current_token_;
}

char TokenIterator::nextCharacter() {
  return input_[++current_input_pos_];
}

char TokenIterator::peekCharacter() {
  return input_[current_input_pos_];
}

#define TTENTRY0(trie, mt) trie[mt] = {.main_type = mt}
#define TTENTRY1(trie, mt, c0, t0) \
  trie[mt] = {.main_type = mt, .size = 1, .variants = {{.c = c0, .t = t0}}}
#define TTENTRY2(trie, mt, c1, t1, c2, t2) \
  trie[mt] = {.main_type = mt,             \
      .size = 2,                           \
      .variants = {{.c = c1, .t = t1}, {.c = c2, .t = t2}}}

// Token table that powers table-driven scanning of simple tokens like ~ , < >
// <= and so on. Implements a trie, a near to hash table structure. Provides
// operator[] to look up an entry
class TokenTable {
  // The table consist of entries. Each entry represents one token or several
  // variants of tokens, based on the first character. For example, if the
  // tokenizer meets '<' in the source code, it has two variants of what token
  // could be scanned: '<' or '<='. In this case we call '<' the main character
  // and the type of token it represents, TOKEN_LESS is the main token type.
  // Depending on which character goes next, we could have other variants, like
  // <=
  struct Entry {
    // Main token type, like if the next character doesn't compose with the
    // current one, making some token
    LuaTokenType main_type;
    // Actual number of variants
    size_t size;
    // List of variants of tokens based on the main type. For example, if we
    // have '<' as a main character and the next character is '=', then
    // TOKEN_LESS_EQUAL is the actual token type, and pair ('=',
    // TOKEN_LESS_EQUAL) is a variant. I put max number of variants as 2 because
    // it's lua and there are no more than 2 additional variants
    struct {
      // Next character after the main character
      char c;
      // Token type that next and main character compose to
      LuaTokenType t;
    } variants[2];
  };

  Entry trie_[TOKEN_LEFT_BRACKET];

  size_t idx(char c) const noexcept {
    switch (c) {
      case '~': return static_cast<size_t>(TOKEN_BNOT);
      case '<': return static_cast<size_t>(TOKEN_LESS);
      case '>': return static_cast<size_t>(TOKEN_BIGGER);
      case '=': return static_cast<size_t>(TOKEN_ASSIGN);
      case ':': return static_cast<size_t>(TOKEN_COLON);
      case '+': return static_cast<size_t>(TOKEN_PLUS);
      case '-': return static_cast<size_t>(TOKEN_MINUS);
      case '*': return static_cast<size_t>(TOKEN_ASTERISK);
      case '%': return static_cast<size_t>(TOKEN_MOD);
      case '^': return static_cast<size_t>(TOKEN_BXOR);
      case '#': return static_cast<size_t>(TOKEN_DASH);
      case '&': return static_cast<size_t>(TOKEN_AT);
      case '|': return static_cast<size_t>(TOKEN_BOR);
      case '(': return static_cast<size_t>(TOKEN_LEFT_PAREN);
      case ')': return static_cast<size_t>(TOKEN_RIGHT_PAREN);
      case '{': return static_cast<size_t>(TOKEN_LEFT_BRACE);
      case '}': return static_cast<size_t>(TOKEN_RIGHT_BRACE);
      case ';': return static_cast<size_t>(TOKEN_SEMICOLON);
      case ',': return static_cast<size_t>(TOKEN_COMMA);
      case ']': return static_cast<size_t>(TOKEN_RIGHT_BRACKET);
    }
    return 0;
  }

 public:
  consteval TokenTable() {
    TTENTRY1(trie_, TOKEN_DIVIDE, '/', TOKEN_DIV);
    TTENTRY1(trie_, TOKEN_BNOT, '=', TOKEN_BNOT_ASSIGN);
    TTENTRY2(trie_, TOKEN_LESS, '<', TOKEN_BLEFT, '=', TOKEN_LESS_EQUAL);
    TTENTRY2(trie_, TOKEN_BIGGER, '>', TOKEN_BRIGHT, '=', TOKEN_BIGGER_EQUAL);
    TTENTRY1(trie_, TOKEN_ASSIGN, '=', TOKEN_EQUALS);
    TTENTRY1(trie_, TOKEN_COLON, ':', TOKEN_COLON_COLON);
    TTENTRY0(trie_, TOKEN_PLUS);
    TTENTRY1(trie_, TOKEN_MINUS, '-', TOKEN_COMMENT);
    TTENTRY0(trie_, TOKEN_ASTERISK);
    TTENTRY0(trie_, TOKEN_MOD);
    TTENTRY0(trie_, TOKEN_BXOR);
    TTENTRY0(trie_, TOKEN_DASH);
    TTENTRY0(trie_, TOKEN_AT);
    TTENTRY0(trie_, TOKEN_BOR);
    TTENTRY0(trie_, TOKEN_LEFT_PAREN);
    TTENTRY0(trie_, TOKEN_RIGHT_PAREN);
    TTENTRY0(trie_, TOKEN_LEFT_BRACE);
    TTENTRY0(trie_, TOKEN_RIGHT_BRACE);
    TTENTRY0(trie_, TOKEN_SEMICOLON);
    TTENTRY0(trie_, TOKEN_COMMA);
    TTENTRY0(trie_, TOKEN_RIGHT_BRACKET);
  }

  const Entry &operator[](char c) const noexcept { return trie_[idx(c)]; }
};

void TokenIterator::recognizeTokensWithTable() {
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
  struct CharTokenPair {
    char c;
    LuaTokenType t;
  };
  struct ScannerTableEntry {
    LuaTokenType main_type;
    size_t size;
    CharTokenPair pairs[2];
  };
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
      STENTRY0(TOKEN_RIGHT_BRACE), STENTRY0(TOKEN_SEMICOLON),
      STENTRY0(TOKEN_COMMA), STENTRY0(TOKEN_RIGHT_BRACKET)};

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
    case ';': table_index = TOKEN_SEMICOLON; break;
    case ',': table_index = TOKEN_COMMA; break;
    case ']': table_index = TOKEN_RIGHT_BRACKET; break;
  }

  const auto &entry = scanner_table[table_index];
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

namespace {
struct ScanIntegerResult {
  double magnitude;
  double power_of_base;
};

ScanIntegerResult scanInteger(const char *str, size_t len, double base) {
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
}  // namespace

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
      case '[': {
        const char *start = input_ + current_input_pos_;
        const auto next_index = current_input_pos_ + 1;
        if (next_index < input_size_) {
          if (input_[next_index] != '[') {
            current_token_.type = TOKEN_LEFT_BRACKET;
          }
          current_input_pos_ += 2;
        } else {
          throw RuntimeError("Unexpected EOF");
        }

        while (current_input_pos_ < input_size_ &&
               input_[current_input_pos_] != ']') {
          ++current_input_pos_;
        }

        if (current_input_pos_ + 1 < input_size_) {
          ++current_input_pos_;
          if (input_[current_input_pos_] == ']') {
            current_token_.type = TOKEN_LONG_STRING_LITERAL;
            const char *new_string = string_table_.InsertString(
                start + 2, input_ + current_input_pos_ - start - 3);
            current_token_.value.string_literal = new_string;
            ++current_input_pos_;
            return;
          }
        }

        unexpectedCharacter();

      } break;
      case '"':
      case '\'': {
        const char *start = input_ + current_input_pos_;
        ++current_input_pos_;
        while (current_input_pos_ < input_size_ &&
               input_[current_input_pos_] != c) {
          ++current_input_pos_;
        }

        if (input_[current_input_pos_] != c) {
          unexpectedCharacter();
        }
        ++current_input_pos_;

        const size_t len = input_ + current_input_pos_ - start - 2;
        const char *string = string_table_.InsertString(start + 1, len);
        current_token_.type = TOKEN_SHORT_STRING_LITERAL;
        current_token_.value.string_literal = string;
        return;
      } break;
      case '.': {
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

        if (isDigit(c)) {
          current_token_.type = TOKEN_NUMBER;
          const auto res = TryEvaluateNumber(input_ + current_input_pos_ - 1,
              input_size_ - current_input_pos_ + 1);
          if (res) {
            current_token_.value.number = res.number;
            current_input_pos_ += res.len;
            return;
          } else {
          }
        }
        return;
      }
      case LUACOMP_DIGIT_CHAR: {
        current_token_.type = TOKEN_NUMBER;
        const auto res = TryEvaluateNumber(
            input_ + current_input_pos_, input_size_ - current_input_pos_);
        if (res) {
          current_token_.value.number = res.number;
          current_input_pos_ += res.len;
        } else {
        }
        return;
      }
      case LUACOMP_TOKEN: {
        recognizeTokensWithTable();
        if (current_token_.type == TOKEN_COMMENT) {
          while (input_[++current_input_pos_] != '\n' &&
                 current_input_pos_ < input_size_)
            ;
        } else {
          return;
        }
      }
      case LUACOMP_ALPHA_CHAR: {
        static constinit const auto kwtable = KeywordsTable();
        const auto [str, len] = scanString(isKeywordCharacter);
        LuaTokenType token_type = kwtable[String(str, len)];
        current_token_.type = token_type;
        if (token_type == TOKEN_IDENTIFIER) {
          const auto string = string_table_.InsertString(str, len);
          current_token_.value.identifier = string;
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

TokenIterator1::TokenIterator1(const char *input_name, String input)
    : input_name_(input_name),
      input_(input),
      input_iter_(input_),
      string_table_(128) {}

const Token &TokenIterator1::operator*() const noexcept {
  return current_token_;
}

TokenIterator1::operator bool() const noexcept {
  return current_token_.type != TOKEN_INVALID && !!input_iter_;
}

static constinit const auto kwtable = KeywordsTable();
static constinit const auto toktable = TokenTable();

const TokenIterator1 &TokenIterator1::operator++() {
TokenIterator1_tokenization_start:
  switch (input_iter_.Peek()) {
    // Scans either a keyword or an identifer. The algorithm is as follows:
    // 1. Save the current iterator that points to the first character in a word
    // 2. Iterate over the input string until non-alphanumeric character is met
    // 3. After iterating, the current iterator points to the character next to
    // the last alphanumeric character. If the end of the input has reached, the
    // iterator points to a byte that is next to the last byte.
    // 4. The word is obtained as the difference between current iterator and
    // iterator saved at step 1
    // 5. If the keywords table recognizes the keyword, we have a token type
    // 6. Otherwise, we have some identifier and we insert it to the string
    // table to intern it
    // 7. End. Next tokenization iteration will start with the character that is
    // next to the scanned word
    case LUACOMP_ALPHA_CHAR: {
      const auto start = input_iter_;
      input_iter_.IterateWhile(isKeywordCharacter);
      const auto word = input_iter_ - start;
      current_token_.type = kwtable[word];
      if (current_token_.type == TOKEN_IDENTIFIER) {
        current_token_.value.identifier = string_table_.Insert(word);
      }
    } break;
    // Recognizes usual tokens like < <= >= and so on.
    // 1. Looking up the token table for the current character, obtaining the
    // corresponding table entry
    // 2. Assign the token type from the entry
    // 3. Getting the next character and iterating over the variants checking
    // whether this character matches any
    // 4. If there's a match, assigning the final token type and incrementing
    // the input iterator so that the next tokenization iteration will start
    // with the next character
    // 5. Otherwise, we already assigned the actual token type and incremented
    // the iterator.
    // 6. If the final token is a comment, we have to skip the entire line
    // 7. End
    case LUACOMP_TOKEN: {
      const auto &entry = toktable[input_iter_.Peek()];
      current_token_.type = entry.main_type;
      char c = input_iter_.Next();
      for (size_t i = 0; i < entry.size; ++i) {
        if (c == entry.variants[i].c) {
          current_token_.type = entry.variants[i].t;
          input_iter_.Next();
          break;
        }
      }
      // Skipping the line in case of comment
      if (current_token_.type == TOKEN_COMMENT) {
        input_iter_.IterateWhile([](char c) { return c != '\n'; });
        // We have to skip comments. This goto will trigger a jump to case '\n',
        // which is exactly what we need
        goto TokenIterator1_tokenization_start;
      }
    } break;
    case '.': {
      current_token_.type = TOKEN_PERIOD;
      if (input_iter_.Next() == '.') {
        current_token_.type = TOKEN_2PERIOD;
        if (input_iter_.Next() == '.') {
          current_token_.type = TOKEN_3PERIOD;
          input_iter_.Next();
        }
      }
    } break;
    case '\n': {
      ++line_number_;
    }
    case ' ':
    case '\t':
    case '\r': {
      input_iter_.Next();
      goto TokenIterator1_tokenization_start;
    } break;
  }
  return *this;
}