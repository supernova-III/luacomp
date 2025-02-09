#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include "lib.hh"
#include "tokenizer.hh"

struct File {
  std::ifstream stream;
  std::vector<char> data;

  File(const std::filesystem::path& path)
      : data(std::filesystem::file_size(path)), stream(path) {
    // stream.exceptions(std::ifstream::failbit);
    stream.read(data.data(), data.size());
  }

  String Content() const { return String(data.data(), data.size()); }
};

int main(int argc, char* argv[]) try {
  if (argc < 2) {
    throw RuntimeError("Usage: %s <source_file>", argv[0]);
  }

  auto file = File(argv[1]);
  auto iter = TokenIterator(argv[1], file.Content());
  while (++iter) {
    const auto& current = *iter;
    if (current.type == LuaTokenType::TOKEN_NUMBER) {
      std::cout << current.value.number << std::endl;
    }
  }
  return 0;
} catch (const std::runtime_error& e) {
  std::cerr << e.what() << std::endl;
} catch (const RuntimeError& e) {
  e.PrintToStdOut();
}
