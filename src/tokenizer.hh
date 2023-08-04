#pragma once
#include <unordered_set>
#include <string>

enum LuaTokenType : uint32_t {
  TOKEN_DIVIDE,         // ok
  TOKEN_BNOT,           // ok
  TOKEN_LESS,           // ok
  TOKEN_BIGGER,         // ok
  TOKEN_ASSIGN,         // ok
  TOKEN_COLON,          // ok
  TOKEN_PLUS,           // ok
  TOKEN_MINUS,          // ok
  TOKEN_ASTERISK,       // ok
  TOKEN_MOD,            // ok
  TOKEN_BXOR,           // ok
  TOKEN_DASH,           // ok
  TOKEN_AT,             // ok
  TOKEN_BOR,            // ok
  TOKEN_LEFT_PAREN,     // ok
  TOKEN_RIGHT_PAREN,    // ok
  TOKEN_LEFT_BRACE,     // ok
  TOKEN_RIGHT_BRACE,    // ok
  TOKEN_LEFT_BRACKET,   // ok
  TOKEN_RIGHT_BRACKET,  // ok
  TOKEN_SEMICOLON,      // ok
  TOKEN_COMMA,          // ok
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
  TOKEN_COLON_COLON,   // ok
  TOKEN_BLEFT,         // ok
  TOKEN_BRIGHT,        // ok
  TOKEN_DIV,           // ok
  TOKEN_EQUALS,        // ok
  TOKEN_BNOT_ASSIGN,   // ok
  TOKEN_LESS_EQUAL,    // ok
  TOKEN_BIGGER_EQUAL,  // ok
  TOKEN_PERIOD,        // ok
  TOKEN_2PERIOD,       // ok
  TOKEN_3PERIOD,       // ok
  TOKEN_COMMENT,       // ok
  TOKEN_LONG_STRING_LITERAL,
  TOKEN_SHORT_STRING_LITERAL,

  // clang-format off
  // LuaNumber          -> ReducedDecimalForm | HexForm |  FullDecimalForm
  // FullDecimalForm    -> [0-9]+(\.[0-9]?)?([eE][+-]?[0-9]+)?$
  // ReducedDecimalForm -> \.[0-9]+([eE][+-]?[0-9]+)?$
  // HexForm            -> (0[xX])[0-9a-fA-F]+(\.[0-9a-fA-F]*)*([pP][+-]?[0-9]+)?$
  // clang-format on
  TOKEN_NUMBER,
  TOKEN_IDENTIFIER,
  TOKEN_END_OF_STREAM  // ok
};

bool IsAlpha(char c);

bool IsHexChar(char c);

bool IsHexadecimal(char c);

double HexToNumber(char c);

double CharToDigit(char c);

bool IsDigit(char c);

bool IsKeywordCharacter(char c);

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

// Enumerates exponent types
enum struct ExponentType : uint8_t {
  // no exponent
  NONE,
  // like E12
  PLAIN,
  // like E+12
  PLUS,
  // like E-12
  MINUS
};

using TransformingFunction = double (*)(char);
using CheckingFunction = bool (*)(char c);

// Data required for quick number evaluation
struct EvaluateNumberArgs {
  // A string that represents a number. For hex numbers, 0x and 0X have to be
  // omitted.
  const char* string;
  // Length of a string that represents a number. Corresponds to the next
  // position to the latest character representing a number
  size_t len;
  // Base of a number
  NumberBase base = NumberBase::Dec();
  // A function that transorms character to a digit. For dec numbers it will be
  // CharToDigit, for hex it will be HexadecimalToNumber
  TransformingFunction transform;
  // Position of a floating point dot. Equals to len if not present
  size_t dot_position;
  // Type of an exponent. It's needed to safely skip E, E+ or E- and get right
  // to the power
  ExponentType exponent_type;
  // Position of an exponent. Equals to len if not present
  size_t exponent_position;
};

// Evaluates a number represented by a string, according to the Lua spec. This
// function does not do any checking/scanning, so it's entirely relying on the
// provided data. If the provided data doesn't correspond to what is really
// contained in a string, the behavior is undefined
double EvaluateNumber(const EvaluateNumberArgs& args);

enum struct ScanNumberError {
  OK,
  UNEXPECTED_END,
  NO_NUMBER
};

// Recognize a number in a string. If returned value is OK, then the length of a
// substring containing a valid number representation is set to
// match_substring_len
ScanNumberError ScanNumber(
    const char* string, size_t len, size_t& matched_substring_len);

// Recognizes a number, collecting all information required for quick evaluation
// of a number
ScanNumberError ScanNumber(
    const char* string, size_t size, EvaluateNumberArgs& evaluator_args);

// Scans and evaluates a number in a string. If scan fail, an error code will be
// place into error parameter
double ScanEndEvaluateNumber(
    const char* string, size_t len, ScanNumberError& error);

struct Token {
  LuaTokenType type = TOKEN_END_OF_STREAM;
  union {
    const char* identifier;
    const char* string_literal;
    double number;
  } value;
};

// Iterates over provided string, scanning tokens. It doesn't store tokens, it
// can only provide the latest scanned token.
struct TokenIterator {
  // Input stream
  const char* input_ = nullptr;
  const char* input_name_ = nullptr;
  // Size of an input stream
  size_t input_size_ = 0;
  // Current line number in a source file
  size_t line_number_ = 1;
  // Position of a character being recognized
  size_t current_input_pos_ = 0;
  // Last scanned token
  Token current_token_ = {};

  std::unordered_set<std::string> string_table_;

  // Should be used to create tokenizer for an input string.
  TokenIterator(const char* input_name, const char* input, size_t size);

  // When the input string exhausted, this function can be used to provide
  // another string to tokenize
  void UpdateInput(const char* new_input);

  // Scan for the next token
  TokenIterator& operator++();

  // Peek the latest scanned token
  const Token& operator*() const;

  // Iterator is valid if an input stream is not exhausted or the latest scanned
  // token was not invalid
  operator bool() const;

 private:
  // The actual function that does tokenization
  void nextToken();

  // Do table-driven scan for certain characters
  void recognizeTokensWithTable();

  // Advance input iterator and get next character
  char nextCharacter();

  // Get current character
  char peekCharacter();

  void unexpectedCharacter();

  // Generic function to deduplicate code that scans strings
  struct ScanStringResult {
    const char* str;
    size_t len;
  };
  ScanStringResult scanString(CheckingFunction f);

  // Recognize exponent
  double recognizeExponent(CheckingFunction checker, double exp_base);
};
