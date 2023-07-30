#include "tokenizer.cpp"
#include <windows.h>
#include <system_error>
#include <string_view>
#include <format>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <expected>
#include <iterator>
#include <vector>

// Exception that shows up OS error message.
class SystemError final : public std::runtime_error {
 public:
  SystemError(const std::string& message)
      : std::runtime_error(std::format("{}: {}", message,
            std::error_code(::GetLastError(), std::system_category())
                .message())) {}
};

void What(const std::runtime_error& e) {
  std::printf("%s\n", e.what());
}

int main(int argc, char* argv[]) try {
  if (argc != 2) {
    throw std::runtime_error(std::format("Usage: {} <source_file>", argv[0]));
  }

  auto file = std::ifstream(argv[1], std::ios::in);
  if (!file) {
    throw SystemError(std::format("Unable to create file {}:", argv[1]));
  }
  const auto data = std::vector<char>(std::istreambuf_iterator<char>(file), {});

  auto iter = TokenIterator(argv[1], data.data(), data.size());
  while (++iter) {
    auto current = *iter;
  }
  return 0;
} catch (const SystemError& e) {
  What(e);
} catch (const std::runtime_error& e) {
  What(e);
}
