#include "tokenizer_number_evaluation.hh"
#include "tokenizer_utils.hh"

namespace {
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
}  // namespace

EvaluateNumberResult TryEvaluateNumber(const char* string, size_t len) {
  const char* cur = string;
  auto exhausted = [&](const char* cur) { return cur - string >= len; };
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

  CheckingFunction checker = IsDigit;
  CheckingFunction exponent_checker = [](char c) {
    return c == 'e' || c == 'E';
  };
  TransformingFunction transform = CharToDigit;
  if (base == NumberBase::Hex()) {
    checker = IsHexadecimal;
    exponent_checker = [](char c) { return c == 'p' || c == 'P'; };
    transform = HexToNumber;
  }

  size_t integer_part_len = 0;
  const char* integer_part_str = cur;
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
  const char* fractional_part_str = cur;
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
      const char* exponent_number_str = cur;
      if (checker(*cur)) {
        while (!exhausted(cur) && IsDigit(*cur)) {
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
              NumberBase::Dec(), CharToDigit)
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
