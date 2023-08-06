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
