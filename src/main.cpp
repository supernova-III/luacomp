#include <cstdio>
#include <cstdlib>
#include "lib.hh"
#include "tokenizer.hh"

struct File {
  char* buffer;
  size_t size;
  FILE* handle;

  File(const char* path) {
    auto error = fopen_s(&handle, path, "rb");
    if (error) {
      throw SystemError("Unable to create file %s", path);
    }
    if (fseek(handle, 0, SEEK_END)) {
      throw SystemError("Unable to get input file size.");
    }
    const auto file_size = ftell(handle);
    fseek(handle, 0, 0);
    buffer = static_cast<char*>(calloc(file_size + 1, 1));
    if (!buffer) {
      throw RuntimeError("Unable to allocate memory to read input file.");
    }
    size = fread(buffer, 1, file_size, handle);
    if (size != file_size) {
      throw SystemError("Unable to read input file %s", path);
    }
  }

  File() = delete;
  File(const File&) = delete;
  File& operator=(const File&) = delete;
  File(File&&) = default;
  File& operator=(File&&) = default;

  String Content() const { return String(buffer, size); }

  ~File() {
    if (buffer) {
      free(buffer);
    }
    if (handle) {
      fclose(handle);
    }
  }

int main(int argc, char* argv[]) try {
  if (argc < 2) {
    throw RuntimeError("Usage: %s <source_file>", argv[0]);
  }

  auto file = File(argv[1]);
  auto iter = TokenIterator(argv[1], file.Content());
  while (++iter) {
    auto current = *iter;
  }
  return 0;
} catch (const RuntimeError& e) {
  e.PrintToStdOut();
}
