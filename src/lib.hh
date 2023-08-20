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
  RuntimeError(const char *format, ...);

  const char *What() const { return what_; }
  void PrintToStdOut() const { std::printf("%s\n", what_); }
};

class SystemError final : public RuntimeError {
 public:
  SystemError(const char *format, ...);
};

// Allocator that allocates memory from buffers that connected via linked
// list, to avoid expensive reallocation when another buffer is full. This
// allocator never frees the memory.
class PoolAllocator {
  // Represends single pool. Pool is like std::vector, but also contains the
  // pointer to another pool. It's to be allocated together with the actual pool
  // memory, so this struct is actually a header of a pool
  struct PoolHeader {
    PoolHeader *prev;
    size_t size;
    size_t capacity;
    uint8_t *memory;
  };

  // Pool that is not full. All previous pools are full
  PoolHeader *current_pool_;
  size_t n_pools_;

 public:
  static constexpr size_t MaxNumberOfPools() { return 4; }
  static constexpr size_t MaxPoolCapacity() { return 4 * 1024; }

  // Initializes object, allocating the first pool with the given capacity
  PoolAllocator(size_t size = MaxPoolCapacity());

  // Allocates memory of the given size from the pool
  uint8_t *Allocate(size_t size);

 private:
  // Allocates new pool, making it current
  void allocatePool(size_t capacity);
  // Reallocates current pool, so that new capacity equals to doubled size of
  // the current buffer. If size_hint is bigger than double size of the current
  // buffer, new capacity will be equal to size_hint
  void reallocateCurrentPool(size_t size_hint);
};

using StringHashFunction = size_t (*)(const char *string, size_t len);

struct String {
  const char *data;
  size_t len;

  constexpr String(const char *string, size_t len) noexcept
      : data(string), len(len) {}
  // constructor for c-strings
  String(const char *c_string) noexcept
      : data(c_string), len(strlen(c_string)) {}
  String() = default;
  // Assign operator for c-strings
  String &operator=(const char *c_string) noexcept;
  // comparison with c-strings
  bool operator==(const char *c_string) const noexcept;
  // comparison with String
  bool operator==(const String &other) const noexcept;
  // returns character at index
  char operator[](size_t index) const noexcept;
};

class StringIterator {
  size_t pos_;
  String &string_;

 public:
  StringIterator(String &string) noexcept : string_(string), pos_() {}

  // Peeks current element
  char Peek() const noexcept;

  // Checks if an iterator is valid. An iterator is valid if its pos if less
  // than the length of a string it iterates over.
  operator bool() const noexcept;

  // Returns a string which is a difference between two iterators. For example,
  // if we have a string abcdef and it1 points to c, and it2 points to f, the
  // difference is cde.
  String operator-(const StringIterator &other) const;

  // Keeps iterating until the target character met. Increments iterator first.
  // Returns true if the target character met, false otherwise
  bool IterateTo(char target) noexcept;

  template <typename F>
  void IterateWhile(F f) noexcept {
    for (auto c = Next(); !!(*this) && f(c); c = Next())
      ;
  }

  // Returns next character or 0 if the end of the string reached
  char Next() noexcept;

 private:
  // Increments iterator
  StringIterator &increment() noexcept;
};

class StringTable {
  struct Node {
    Node *prev;
    size_t len;
    char string;

    char *GetStringToModify() { return &string; }
    const char *GetString() const { return &string; }
  };

  PoolAllocator allocator_ = {};
  StringHashFunction hasher_ = [](const char *str, size_t len) -> size_t {
    size_t hash = 5381;
    for (size_t i = 0; i < len; ++i) {
      hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
  };
  size_t hash_seed_;
  // array of linked list. Problem: if this memory is allocated with allocator_,
  // it cannot be reallocated
  Node **buckets_ = nullptr;
  // number of chains
  size_t capacity_;
  size_t size_ = 0;
  double max_load_factor_ = 0.5;

 public:
  StringTable(size_t capacity);

  const char *Insert(const char *string, size_t len);
  const char *Insert(const String &string);

 private:
  Node *allocateStringNode(const char *str, size_t len);
};
