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

double EvaluateNumber(const EvaluateNumberArgs& args) {
  const auto [integer_part, _] = evaluateIntegerFromString(
      args.string, args.dot_position, args.base, args.transform);
  if (args.dot_position == args.len - 1 || args.dot_position == args.len) {
    return integer_part;
  }

  // 123.123e12
  // dot_pos = 3
  // exponent_pos = 7
  // len(123) = exponent_pos - dot_pos - 1
  const auto fractional_part_len =
      args.exponent_position - args.dot_position - 1;
  double fractional_part = 0;
  const auto [res, power] =
      evaluateIntegerFromString(args.string + args.dot_position + 1,
          fractional_part_len, args.base, args.transform);
  fractional_part = res / power;

  size_t next_pos = 0;
  double sign = 1;
  const double exponent_base = 10 - 8 * (args.base == NumberBase::Hex());
  switch (args.exponent_type) {
    case ExponentType::NONE: return integer_part + fractional_part;
    case ExponentType::PLAIN: next_pos = args.exponent_position + 1; break;
    case ExponentType::MINUS:
    case ExponentType::PLUS: {
      next_pos = args.exponent_position + 2;
      sign -= 2 * (args.exponent_type == ExponentType::MINUS);
    }
  }

  const size_t exponent_number_len = args.len - next_pos;
  const auto [exponent_number, __] = evaluateIntegerFromString(
      args.string + next_pos, exponent_number_len, args.base, args.transform);
  double exponent_part = 1;
  for (size_t i = 0; i < exponent_number; ++i) {
    exponent_part *= exponent_base;
  }

  if (sign < 0) {
    exponent_part = 1 / exponent_part;
  }

  return (integer_part + fractional_part) * exponent_part;
}

ScanNumberError ScanNumber(
    const char* string, size_t len, EvaluateNumberArgs& evaluator_args) {
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

  evaluator_args.string = cur;

  CheckingFunction checker = IsDigit;
  CheckingFunction exponent_checker = [](char c) {
    return c == 'e' || c == 'E';
  };
  if (base.val == NumberBase::Enum::HEX) {
    checker = IsHexadecimal;
    exponent_checker = [](char c) { return c == 'p' || c == 'P'; };
  }

  size_t integer_part_len = 0;
  while (!exhausted(cur) && checker(*cur)) {
    ++cur;
    ++integer_part_len;
  }

  if (*cur == '.') {
    evaluator_args.dot_position = cur - string;
    ++cur;
  }

  if (exhausted(cur)) {
    if (integer_part_len == 0) {
      return ScanNumberError::UNEXPECTED_END;
    }
    evaluator_args.len = cur - string;
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
    evaluator_args.exponent_position = cur - string;
    ++cur;
    if (!exhausted(cur)) {
      if (*cur == '+' || *cur == '-') {
        evaluator_args.exponent_type = ExponentType::PLUS;
        if (*cur == '-') {
          evaluator_args.exponent_type = ExponentType::MINUS;
        }
        ++cur;
        if (exhausted(cur)) {
          return ScanNumberError::UNEXPECTED_END;
        }
      } else {
        evaluator_args.exponent_type = ExponentType::PLAIN;
      }
      if (checker(*cur)) {
        while (!exhausted(cur) && IsDigit(*cur)) {
          ++cur;
        }
      } else {
        return ScanNumberError::UNEXPECTED_END;
      }
    }
  }
  evaluator_args.len = cur - string;
  return {};
}

double ScanEndEvaluateNumber(
    const char* string, size_t len, ScanNumberError& error) {
  EvaluateNumberArgs args = {};
  error = ScanNumber(string, len, args);
  if (error == ScanNumberError::OK) {
    return EvaluateNumber(args);
  }
  return {};
}

ScanNumberError ScanNumber(
    const char* string, size_t len, size_t& matched_substring_len) {
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
  if (base == NumberBase::Hex()) {
    checker = IsHexadecimal;
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
        while (!exhausted(cur) && IsDigit(*cur)) {
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
