#include "tokenizer.cpp"

#include <windows.h>

struct SystemError {
  const char* message;
  ::DWORD code;

  static SystemError Last();
};

SystemError SystemError::Last() {
  static char buffer[64 * 1024] = {};
  ::DWORD error_code = ::GetLastError();
  ::DWORD size = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buffer, 64 * 1024, NULL);
  if (size == 0) {
    Panic("FATAL ERROR: %lu\n", ::GetLastError());
  }
  buffer[size] = 0;
  return {buffer, error_code};
}

namespace {
::HANDLE openSourceFile(const char* file_name) {
  // TODO: do unbuffered input potentially. Now it's not very good time to do
  // this because it requires unnecessary efforts to ensure proper memory
  // alignement. There's my note about it:
  // to use unbuffered input, the memory must be sector-aligned. If the
  // sector size is less than the memory page size, VirtualAlloc can be used
  // to allocate properly aligned memory. Otherwise, the memory must be
  // aligned manually. To check the sector size, use GetDistFreeSpace
  // function. To check the memory page size, use GetSystemInformation
  // function
  ::HANDLE handle = ::CreateFileA(file_name, GENERIC_READ, 0, NULL,
      OPEN_EXISTING, /*FILE_FLAG_NO_BUFFERING*/ NULL, NULL);

  if (handle == INVALID_HANDLE_VALUE) {
    SystemError error = SystemError::Last();
    Panic("Could not open file %s. Error code is %lu: %s", file_name,
        error.code, error.message);
  }
  return handle;
}
}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    Panic("Usage: %s <source_file>\n", argv[0]);
  }

  ::HANDLE source_file_handle = openSourceFile(argv[1]);
  ::DWORD file_size = ::GetFileSize(source_file_handle, NULL);
  auto memory = static_cast<char*>(::VirtualAlloc(
      NULL, file_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
  if (!memory) {
    auto error = SystemError::Last();
    Panic(
        "No enough memory to read source file %s: %s", argv[1], error.message);
  }
  ::DWORD read = 0;
  if (::ReadFile(source_file_handle, memory, file_size, &read, NULL) != TRUE) {
    auto error = SystemError::Last();
    Panic("Unable to read source file %s: %s", argv[1], error.message);
  }

  auto iter = TokenIterator::New(argv[1], memory, file_size);
  while (++iter) {
    auto current = *iter;
  }
  return 0;
}
