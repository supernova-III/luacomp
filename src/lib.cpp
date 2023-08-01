#include "lib.hh"
#include <cstdlib>

#define FORMAT_MESSAGE(format, what, szvar)                 \
  va_list args;                                             \
  va_start(args, format);                                   \
  auto szvar = vsnprintf(what, sizeof(what), format, args); \
  va_end(args);

RuntimeError::RuntimeError(const char* format, ...) {
  FORMAT_MESSAGE(format, what_, _);
}

SystemError::SystemError(const char* format, ...) {
  static char what[256] = {};
  FORMAT_MESSAGE(format, what, size);
  const char* first = what;
  char* second = what + size + 1;
  strerror_s(second, sizeof(what) - size - 1, errno);
  snprintf(what_, sizeof(what_), "%s: %s\n", first, second);
}

PoolAllocator::PoolAllocator(size_t size)
    : current_pool_(nullptr), n_pools_(0) {
  allocatePool(size);
}

#undef FORMAT_MESSAGE

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
    throw RuntimeError(
        "Memory pool size cannot be bigger than %llu bytes", MaxPoolCapacity());
  }
  if (n_pools_ == 4) {
    throw RuntimeError("Memory pools number limit exceeded");
  }
  const size_t allocation_size = sizeof(PoolHeader) + capacity;
  auto header = reinterpret_cast<PoolHeader*>(malloc(allocation_size));
  if (!header) {
    throw RuntimeError("Not enough memory to allocate memory pool");
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
    throw RuntimeError("Not enough memory to allocate memory pool");
  }
  header->capacity = new_capacity;
}
