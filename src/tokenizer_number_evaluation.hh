#pragma once
#include <stddef.h>
#include <stdint.h>

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

// Function that transforms a character into a digit
using TransformingFunction = double (*)(char);
// Function that checks if a character satisfies specific conditions
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

// Errors recognized when scanning a number
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
