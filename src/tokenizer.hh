#pragma once
#include "lib.hh"

// Function that transforms a character into a digit
using TransformingFunction = double (*)(char);
// Function that checks if a character satisfies specific conditions
using CheckingFunction = bool (*)(char c);

// Errors recognized when scanning a number
enum struct ScanNumberError {
  OK,
  UNEXPECTED_END,
  NO_NUMBER
};

struct EvaluateNumberResult {
  ScanNumberError error = ScanNumberError::OK;
  double number;
  size_t len;

  EvaluateNumberResult(ScanNumberError error) : error(error) {}
  EvaluateNumberResult(double number, size_t len) : number(number), len(len) {}
  operator bool() const { return error == ScanNumberError::OK; }
};

// Evaluates a number represented by a string, according to the Lua spec. This
// function does not do any checking/scanning, so it's entirely relying on the
// provided data. If the provided data doesn't correspond to what is really
// contained in a string, the behavior is undefined
EvaluateNumberResult TryEvaluateNumber(const char* string, size_t len);

enum LuaTokenType : uint32_t {
  TOKEN_DIVIDE,
  TOKEN_BNOT,
  TOKEN_LESS,
  TOKEN_BIGGER,
  TOKEN_ASSIGN,
  TOKEN_COLON,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_ASTERISK,
  TOKEN_MOD,
  TOKEN_BXOR,
  TOKEN_DASH,
  TOKEN_AT,
  TOKEN_BOR,
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,
  TOKEN_RIGHT_BRACKET,
  TOKEN_LEFT_BRACKET,
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
  TOKEN_COLON_COLON,
  TOKEN_BLEFT,
  TOKEN_BRIGHT,
  TOKEN_DIV,
  TOKEN_EQUALS,
  TOKEN_BNOT_ASSIGN,
  TOKEN_LESS_EQUAL,
  TOKEN_BIGGER_EQUAL,
  TOKEN_PERIOD,
  TOKEN_2PERIOD,
  TOKEN_3PERIOD,
  TOKEN_COMMENT,
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
  TOKEN_END_OF_STREAM,
  TOKEN_INVALID
};

struct Token {
  LuaTokenType type = TOKEN_END_OF_STREAM;
  union {
    const char* identifier;
    const char* string_literal;
    double number;
  } value;
};

// Iterates over provided string, recognizing tokens. It's not assumed to store
// all recognized tokens, just the last one.
class TokenIterator1 {
  // The input stream
  String input_;
  // Iterator over the input string
  StringIterator input_iter_;
  // Input stream name (usually means source file name)
  const char* input_name_;
  // Current line number in the input stream
  size_t line_number_ = 0;
  // Last recognized token
  Token current_token_;
  // Table for interning identifiers
  StringTable string_table_;

 public:
  TokenIterator1(const char* input_name, String input);

  // Scans for the next token
  const TokenIterator1& operator++();
  // Peeks the latest recognized token
  const Token& operator*() const noexcept;
  // Returns true if tokenization is not yet finished and there are no errors,
  // false otherwise
  operator bool() const noexcept;
};

// Iterates over provided string, scanning tokens. It doesn't store tokens, it
// can only provide the latest scanned token.
class TokenIterator {
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

  StringTable string_table_;

 public:
  // Should be used to create tokenizer for an input string.
  TokenIterator(const char* input_name, const char* input, size_t size);

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

  // Throws runtime error with the message about unexpected character
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
