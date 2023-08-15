#include "lib.hh"
#include <cstdlib>
#include <cstring>

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
  auto header = reinterpret_cast<PoolHeader*>(calloc(1, allocation_size));
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

StringTable::Node* StringTable::allocateStringNode(
    const char* str, size_t len) {
  const size_t size = len + 1 + sizeof(Node) - sizeof(Node::string);
  auto res = reinterpret_cast<Node*>(allocator_.Allocate(size));
  memcpy(res->GetStringToModify(), str, len);
  res->GetStringToModify()[len] = 0;
  res->len = len;
  return res;
}

StringTable::StringTable(size_t capacity) : capacity_(capacity) {
  auto memory = calloc(capacity, sizeof(Node*));
  if (!memory) {
    throw RuntimeError("Cannot allocate buckets");
  }
  buckets_ = reinterpret_cast<Node**>(memory);
}

const char* StringTable::InsertString(const char* string, size_t len) {
  const double current_load_factor = static_cast<double>(size_) / capacity_;
  if (current_load_factor >= max_load_factor_) {
    const size_t new_capacity = 2 * capacity_;
    auto new_memory = realloc(buckets_, new_capacity);
    if (!new_memory) {
      throw RuntimeError("Cannot reallocate buckets");
    }
    buckets_ = reinterpret_cast<Node**>(new_memory);
  }

  const auto idx = hasher_(string, len) % capacity_;
  auto node = buckets_[idx];
  if (!node) {
    auto new_node = allocateStringNode(string, len);
    buckets_[idx] = new_node;
    ++size_;
  } else {
    auto cur = node;
    while (cur != nullptr) {
      if (cur->len == len && !strncmp(cur->GetString(), string, len)) {
        return cur->GetString();
      }
      cur = cur->prev;
    }
    auto new_node = allocateStringNode(string, len);
    new_node->prev = node;
    buckets_[idx] = new_node;
  }
  return buckets_[idx]->GetString();
}

const char* StringTable::Insert(const String& string) {
  return InsertString(string.data, string.len);
}

String& String::operator=(const char* c_string) noexcept {
  *this = String(c_string);
  return *this;
}

bool String::operator==(const String& other) const noexcept {
  return len == other.len && !strncmp(data, other.data, len);
}

bool String::operator==(const char* c_string) const noexcept {
  return *this == String(c_string);
}

char String::operator[](size_t index) const noexcept {
  return data[index];
}

StringIterator& StringIterator::increment() noexcept {
  ++pos_;
  return *this;
}

StringIterator::operator bool() const noexcept {
  return pos_ < string_.len;
}

String StringIterator::operator-(const StringIterator& other) const {
  if (&string_ != &other.string_) {
    throw RuntimeError("Iterators represent different strings: %p vs. %p",
        &string_, &other.string_);
  }
  if (other.pos_ >= pos_) {
    return {};
  }

  return String(string_.data + other.pos_, pos_ - other.pos_);
}

bool StringIterator::IterateTo(char target) noexcept {
  while (increment() && Peek() != target)
    ;
  return Peek() == target;
}

char StringIterator::Next() noexcept {
  ++pos_;
  return Peek();
}

char StringIterator::Peek() const noexcept {
  return operator bool() * string_[pos_];
}