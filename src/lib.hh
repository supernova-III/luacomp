#pragma once
#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdarg>

class RuntimeError {
 protected:
  char what_[256] = {};

  RuntimeError() = default;

 public:
  RuntimeError(const char* format, ...);

  const char* What() const { return what_; }
  void PrintToStdOut() const { std::printf("%s\n", what_); }
};

class SystemError final : public RuntimeError {
 public:
  SystemError(const char* format, ...);
};

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
