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
#include <stdlib.h>

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

// Allocator that allocates memory from buffers that connected via linked
// list, to avoid expensive reallocation when another buffer is full. This
// allocator never frees the memory.
class PoolAllocator {
  // Represends single pool. Pool is like std::vector, but also contains the
  // pointer to another pool. It's to be allocated together with the actual pool
  // memory, so this struct is actually a header of a pool
  struct PoolHeader {
    PoolHeader* prev;
    size_t size;
    size_t capacity;
    uint8_t* memory;
  };

  // Pool that is not full. All previous pools are full
  PoolHeader* current_pool_;
  size_t n_pools_;

 public:
  static consteval size_t MaxNumberOfPools() { return 4; }
  static consteval size_t MaxPoolCapacity() { return 4 * 1024; }

  // Initializes object, allocating the first pool with the given capacity
  PoolAllocator(size_t size);

  // Allocates memory of the given size from the pool
  uint8_t* Allocate(size_t size);

 private:
  // Allocates new pool, making it current
  void allocatePool(size_t capacity);

  // Reallocates current pool, so that new capacity equals to doubled size of
  // the current buffer. If size_hint is bigger than double size of the current
  // buffer, new capacity will be equal to size_hint
  void reallocateCurrentPool(size_t size_hint);
};

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

PoolAllocator::PoolAllocator(size_t size)
    : current_pool_(nullptr), n_pools_(0) {
  allocatePool(size);
}

uint8_t* PoolAllocator::Allocate(size_t size) {
  const auto new_size = size + current_pool_->size;
  if (new_size > current_pool_->capacity) {
    const auto new_capacity = new_size;
    if (new_capacity > MaxPoolCapacity()) {
      allocatePool(2 * new_capacity);
    } else {
      reallocateCurrentPool(new_capacity);
    }
  }
  const auto result = current_pool_->memory + current_pool_->size;
  current_pool_->size += size;
  return result;
}

void PoolAllocator::allocatePool(size_t capacity) {
  if (capacity > MaxPoolCapacity()) {
    throw std::runtime_error(std::format(
        "Memory pool size cannot be biger than {} bytes.", MaxPoolCapacity()));
  }
  if (n_pools_ == 4) {
    throw std::runtime_error("Memory pools number limit exceeded");
  }
  const size_t allocation_size = sizeof(PoolHeader) + capacity;
  auto header = reinterpret_cast<PoolHeader*>(malloc(allocation_size));
  if (!header) {
    throw std::runtime_error("Not enough memory to allocate memory pool");
  }
  header->prev = current_pool_;
  header->size = 0;
  header->capacity = capacity;
  header->memory = reinterpret_cast<uint8_t*>(header + 1);
  current_pool_ = header;
  n_pools_ += 1;
}

void PoolAllocator::reallocateCurrentPool(size_t size_hint) {
  auto new_capacity = current_pool_->size * 2;
  if (new_capacity < size_hint) {
    new_capacity = size_hint;
  }
  auto header =
      reinterpret_cast<PoolHeader*>(realloc(current_pool_, new_capacity));
  if (!header) {
    throw std::runtime_error("Not enough memory to allocate memory pool");
  }
  header->capacity = new_capacity;
}
