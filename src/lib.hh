#pragma once
#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#define FORMAT_MESSAGE(format, what, szvar)                 \
  va_list args;                                             \
  va_start(args, format);                                   \
  auto szvar = vsnprintf(what, sizeof(what), format, args); \
  va_end(args);

class RuntimeError {
 protected:
  char what_[256] = {};

  RuntimeError() = default;

 public:
  RuntimeError(const char* format, ...) { FORMAT_MESSAGE(format, what_, _); }

  const char* What() const { return what_; }
  void PrintToStdOut() const { std::printf("%s\n", what_); }
};

class SystemError final : public RuntimeError {
 public:
  SystemError(const char* format, ...) {
    static char what[256] = {};
    FORMAT_MESSAGE(format, what, size);
    const char* first = what;
    char* second = what + size + 1;
    strerror_s(second, sizeof(what) - size - 1, errno);
    snprintf(what_, sizeof(what_), "%s: %s\n", first, second);
  }
};

#undef FORMAT_MESSAGE
