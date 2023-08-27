#include "tokenizer.hh"

// Note: scratch allocator is probably to be shared between different compiler
// parts. Will see.
ScratchAllocator *scratch_;

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

}  // namespace

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

#define TTENTRY0(trie, mt) trie[mt] = {.main_type = mt}
#define TTENTRY1(trie, mt, c0, t0) \
  trie[mt] = {.main_type = mt, .size = 1, .variants = {{.c = c0, .t = t0}}}
#define TTENTRY2(trie, mt, c1, t1, c2, t2) \
  trie[mt] = {                             \
      .main_type = mt,                     \
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
}  // namespace

TokenIterator::TokenIterator(const char *input_name, String input)
    : input_(input),
      input_name_(input_name),
      input_iter_(input_),
      string_table_(128) {
  static auto scratch = ScratchAllocator(4 * 1024);
  scratch_ = &scratch;
}

const Token &TokenIterator::operator*() const noexcept {
  return current_token_;
}

TokenIterator::operator bool() const noexcept {
  return current_token_.type != TOKEN_INVALID && !!input_iter_;
}

static constinit const auto kwtable = KeywordsTable();
static constinit const auto toktable = TokenTable();

const TokenIterator &TokenIterator::operator++() {
  // Wrapps call to Next() method of the input iterator, incrementing the column
  // number
  auto NextChar = [&] {
    ++col_;
    return input_iter_.Next();
  };

TokenIterator_tokenization_start:
  char c = input_iter_.Peek();
  switch (c) {
    case '\'':
    case '"': {
      const auto start = input_iter_;
      input_iter_.IterateWhile([&](char current) -> bool {
        ++col_;
        return current != c;
      });
      // It may contain unprocessed escape sequences
      const auto raw_string_literal = input_iter_ - start;
      auto processed_string_literal =
          ProcessRawStringLiteral(raw_string_literal, *scratch_);
      const char *string_literal =
          string_table_.Insert(processed_string_literal);
      current_token_.type = TOKEN_SHORT_STRING_LITERAL;
      current_token_.value.string_literal = string_literal;
    } break;
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
      input_iter_.IterateWhile([&](char c) -> bool {
        ++col_;
        return isKeywordCharacter(c);
      });
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
      char c = NextChar();
      for (size_t i = 0; i < entry.size; ++i, ++col_) {
        if (c == entry.variants[i].c) {
          current_token_.type = entry.variants[i].t;
          NextChar();
          break;
        }
      }
      // Skipping the line in case of comment
      if (current_token_.type == TOKEN_COMMENT) {
        input_iter_.IterateWhile([&](char c) { return c != '\n'; });
        // We have to skip comments. This goto will trigger a jump to case '\n',
        // which is exactly what we need
        goto TokenIterator_tokenization_start;
      }
    } break;
    case '.': {
      current_token_.type = TOKEN_PERIOD;
      if (NextChar() == '.') {
        current_token_.type = TOKEN_2PERIOD;
        if (NextChar() == '.') {
          current_token_.type = TOKEN_3PERIOD;
          NextChar();
        }
      } else if (isDigit(input_iter_.Peek())) {
        current_token_.type = TOKEN_NUMBER;
        current_token_.value.number = tryEvaluateNumber();
      }
    } break;
    case LUACOMP_DIGIT_CHAR: {
      current_token_.type = TOKEN_NUMBER;
      current_token_.value.number = tryEvaluateNumber();
    } break;
    case '\n': {
      ++line_;
      col_ = 0;
      [[fallthrough]];
    }
    case ' ':
    case '\t':
    case '\r': {
      NextChar();
      goto TokenIterator_tokenization_start;
    } break;
  }
  return *this;
}

double TokenIterator::tryEvaluateNumber() {
  StringIterator start = input_iter_;
  const auto [number, error] = EvaluateNumber(input_iter_);
  switch (error) {
    case EvaluateNumberResult::Error::MALFORMED: {
      throw RuntimeError(
          "Lexical error %s:%llu:%llu: malformed number literal", line_, col_);
    }
    default: break;
  }
  // A single number literal doesn't occupy more than 1 line
  col_ += (input_iter_ - start).len;
  return number;
}

namespace {
struct EvaluateIntegerResult {
  double number;
  double power_of_base = 1.0;
};

EvaluateIntegerResult evaluateInteger(const String &string, double base) {
  auto transform = base == 10 ? charToDigit : hexToNumber;
  EvaluateIntegerResult res = {};
  for (size_t i = 0; i < string.len; ++i) {
    const auto index = string.len - i - 1;
    res.number += res.power_of_base * transform(string[index]);
    res.power_of_base *= base;
  }
  return res;
}
}  // namespace

EvaluateNumberResult EvaluateNumber(StringIterator &iter) {
  char c = iter.Peek();
  int base = 10;
  int exponent_base = 10;
  auto checker = isDigit;
  EvaluateNumberResult result = {};
  // Checking for 0x or 0X
  if (c == '0') {
    c = iter.Next();
    if (c == 'x' || c == 'X') {
      // If so, setting helper variables for future evaluation
      base = 16;
      exponent_base = 2;
      checker = isHexadecimal;
      c = iter.Next();
    }
  }

  // Getting integer part
  const StringIterator integer_part_start = iter;
  iter.IterateWhile(checker);
  const String integer_part = iter - integer_part_start;
  const auto [evaluated_integer_part, _] = evaluateInteger(integer_part, base);

  double evaluated_fractional_part = 0;
  // Checking fractional part
  if (iter.Peek() == '.') {
    c = iter.Next();
    const StringIterator fractional_part_start = iter;
    iter.IterateWhile(checker);
    const String fractional_part = iter - fractional_part_start;

    if (integer_part.len == 0 && fractional_part.len == 0) {
      result.error = EvaluateNumberResult::Error::MALFORMED;
      return result;
    }

    auto [res_frac, power_of_base] = evaluateInteger(fractional_part, base);
    evaluated_fractional_part = res_frac / power_of_base;
  } else {
    if (integer_part.len == 0) {
      result.error = EvaluateNumberResult::Error::MALFORMED;
      return result;
    }
  }

  c = iter.Peek();
  double evaluated_exponent = 1;
  if ((exponent_base == 10 && (c == 'e' || c == 'E')) ||
      (exponent_base == 2 && (c == 'p' || c == 'P'))) {
    c = iter.Next();
    int sign = 1;
    if (c == '-' || c == '+') {
      sign -= 2 * (c == '-');
      c = iter.Next();
    }
    const StringIterator exp_part_start = iter;
    iter.IterateWhile(isDigit);
    const String exp_part = iter - exp_part_start;
    if (exp_part.len == 0) {
      result.error = EvaluateNumberResult::Error::INCOMPLETE_EXPONENT;
      return result;
    }
    const auto [exponent_integer, _] = evaluateInteger(exp_part, 10);
    for (size_t i = 0; i < static_cast<size_t>(exponent_integer); ++i) {
      evaluated_exponent *= static_cast<size_t>(exponent_base);
    }
    if (sign < 0) {
      evaluated_exponent = 1 / evaluated_exponent;
    }
  }
  result.number =
      (evaluated_integer_part + evaluated_fractional_part) * evaluated_exponent;
  return result;
}

String ProcessRawStringLiteral(
    const String &string, ScratchAllocator &scratch) {
  StringIterator iter(string);
  auto buffer = (char *)scratch.Allocate(string.len);
  size_t index = 0;
  if (buffer == nullptr) {
    throw RuntimeError(
        "Cannot allocate %llu bytes from the scratch memory", string.len);
  }

  while (iter && index < string.len) {
    char c = iter.Peek();
    if (c != '\\') {
      buffer[index++] = c;
    } else {
      c = iter.Next();
      switch (c) {
        case 'a': buffer[index++] = '\a'; break;
        case 'b': buffer[index++] = '\b'; break;
        case 'f': buffer[index++] = '\f'; break;
        case 'n': buffer[index++] = '\n'; break;
        case 'r': buffer[index++] = '\r'; break;
        case 'v': buffer[index++] = '\v'; break;
        case '\'':
        case '\\':
        case '"': buffer[index++] = c; break;
        case 'x': {
          c = iter.Next();
          size_t i = 0;
          while (isHexadecimal(c) && i < 2) {
            buffer[index++] = c;
            ++i;
            c = iter.Next();
          }
          if (i >= 2) {
            throw RuntimeError("Invalid UTF-8 codepoint");
          }
        } break;
        case 'd': {
          c = iter.Next();
          size_t i = 0;
          while (isDigit(c) && i < 3) {
            buffer[index++] = c;
            ++i;
            c = iter.Next();
          }
          if (i >= 3) {
            throw RuntimeError("Invalid ASCII codepoint");
          }
        } break;
      }
    }
  }
  return String(buffer, index);
}
