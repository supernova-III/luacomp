#pragma once
#include "common.hh"

// Super-mega-default array of bytes
struct PoolAllocator {
  u8* memory_;
  usize size_;
  usize capacity_;

  static PoolAllocator New(usize capacity);
  u8* Allocate(usize size);
};

struct StringNode {
  StringNode* next = nullptr;
  usize size = 0;
  const char* data = nullptr;

  static StringNode* New(
      const char* string, usize string_size, PoolAllocator& allocator);
};

struct StringList {
  StringNode* head = nullptr;
  StringNode* tail = nullptr;

  static StringList New(const char* string, usize size);
  bool AddNode(const char* string, usize size);
};

// Hash table for strings with contiguous layout
// Memory layout
//     -------------------
//     |                 \/
// [ node_1->string_1  node_2->string_2 ... node_n->string_n ] - arena
//     ^   -----------------------------------^
//     |   |
// [ head tail ] - stack
//
struct StringTable {
  f64 max_load_;
  usize hash_seed_;
  usize nbuckets_;
  usize bucketcap_;
  StringList* list_;
};
