#include "lib.cpp"
#include "tokenizer.cpp"
#include <fstream>
#include <iterator>
#include <vector>
#include <string>

int main(int argc, char* argv[]) try {
  double x = std::stod("0x.12efp-2");
  printf("%f\n", x);

  if (argc != 2) {
    throw RuntimeError("Usage: %s <source_file>", argv[0]);
  }

  auto file = std::ifstream(argv[1], std::ios::in);
  if (!file) {
    throw SystemError("Unable to create file %s", argv[1]);
  }
  const auto data = std::vector<char>(std::istreambuf_iterator<char>(file), {});

  auto iter = TokenIterator(argv[1], data.data(), data.size());
  while (++iter) {
    auto current = *iter;
  }
  return 0;
} catch (const RuntimeError& e) {
  e.PrintToStdOut();
}
