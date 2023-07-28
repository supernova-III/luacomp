#include "tokenizer.cpp"

static void validateCommandLineArgs(int argc, char* argv[]) {
  if (argc == 2) {
    return;
  }
  Panic("Usage: %s <source_file>\n", argv[0]);
}

int main(int argc, char* argv[]) {
  validateCommandLineArgs(argc, argv);
  FILE* source = NULL;
  errno_t error = fopen_s(&source, argv[1], "r");
  if (error) {
    Panic("Source file %s could not be opened\n", argv[1]);
  }

  // auto iter = TokenIterator::Create("ASD");
  // while (++iter) {
  //   auto current = *iter;
  // }
  return 0;
}
