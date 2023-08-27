#pragma once
#include "lib.hh"

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

struct EvaluateNumberResult {
  double number;
  enum struct Error {
    OK,
    MALFORMED,
    INCOMPLETE_EXPONENT
  } error = Error::OK;
};

EvaluateNumberResult EvaluateNumber(StringIterator& iter);

struct Token {
  LuaTokenType type = TOKEN_END_OF_STREAM;
  union {
    const char* identifier;
    const char* string_literal;
    double number;
  } value;
};

String ProcessRawStringLiteral(
    const String& string_literal, ScratchAllocator& scratch);

// Iterates over provided string, recognizing tokens. It's not assumed to store
// all recognized tokens, just the last one.
class TokenIterator {
  // The input stream
  String input_;
  // Iterator over the input string
  StringIterator input_iter_;
  // Input stream name (usually means source file name)
  const char* input_name_;
  // Current line number in the input stream
  size_t line_ = 0;
  // Current colon number in the input stream
  size_t col_ = 0;
  // Last recognized token
  Token current_token_;
  // Table for interning identifiers
  StringTable string_table_;

 public:
  TokenIterator(const char* input_name, String input);

  // Scans for the next token
  const TokenIterator& operator++();
  // Peeks the latest recognized token
  const Token& operator*() const noexcept;
  // Returns true if tokenization is not yet finished and there are no errors,
  // false otherwise
  operator bool() const noexcept;

 private:
  // Tries to evaluate a number that is expected in the input stream. Throws on
  // any error
  double tryEvaluateNumber();
};
