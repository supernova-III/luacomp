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
  TOKEN_NUMBER,
  TOKEN_IDENTIFIER,
  TOKEN_END_OF_STREAM  // ok
};

static_assert(TOKEN_3PERIOD != TOKEN_DIVIDE);

// It would be nice to have a token position inside the line of code and the
// line number, if applicable. But it's rather not to be stored here, because
// this data is needed only when some error comes in.
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
  using CheckingFunction = bool (*)(char c);
  ScanStringResult scanString(CheckingFunction f);

  // Recognize exponent
  double recognizeExponent(CheckingFunction checker, double exp_base);
};
