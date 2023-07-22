#pragma once

//
// Character operations
//
static inline bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static inline bool isHexadecimal(char c) {
  return isDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool isBinary(char c) {
  return c == '0' || c == '1';
}

static inline bool isKeywordCharacter(char c) {
  return isAlpha(c) || isDigit(c) || c == '_';
}

static inline int64_t charToDigit(char c) {
  return c - '0';
}

static inline int64_t hexCharToNumber(char c) {
  // convert character to upper case
  if (c >= 'a') {
    c = c - ('A' - 'a');
  }
  return c - 'A' + 10;
}

//
// Keywords hash table
//
typedef struct {
  const char *string;
  size_t len;
  TokenType token_type;
} HashTableEntry;

#define HASH_TABLE_ENTRY(literal, type) \
  { .string = literal, .len = sizeof(literal) - 1, .token_type = type }

static uint32_t hashKeyword(const char *str, size_t len);
static TokenType lookupKeyword(const char *str, size_t len);

//
// Identifiers hash table
//

// Memory pool
typedef struct {
  size_t size;
  size_t capacity;
  uint8_t memory[];
} MemoryPool;

static inline size_t computeMemoryPoolSpace(size_t capacity) {
  return sizeof(MemoryPool) + capacity;
}

static MemoryPool *allocateMemoryPool(size_t capacity);
static uint8_t *allocateFromMemoryPool(MemoryPool **pool, size_t n);

// Hash table
// seed may be 5381
// seed may be 5387
static inline uint32_t hashString(const char *str, size_t len, uint32_t seed) {
  uint32_t h = seed ^ (uint32_t)len;
  for (; len > 0; len--) {
    h ^= ((h << 5) + (h >> 2) + (uint8_t)str[len - 1]);
  }
  return h;
}

typedef StringView Identifier;

static Identifier *allocateIdentifier(const char *str, size_t len);

typedef struct StringNode {
  struct StringNode *next;
  Identifier *identifier;
} StringNode;

typedef struct {
  StringNode *head;
  StringNode *tail;
} StringList;

static StringNode *allocateStringNode();

typedef struct {
  double max_load;
  size_t hash_seed;
  size_t item_size;
  size_t capacity;
  size_t n_buckets;
  StringList buckets[];
} StringTable;

static StringTable *allocateStringTable(size_t capacity);

typedef struct {
  bool collision;
  StringNode *node;
} InsertStringResult;

static InsertStringResult insertStringBasic(
    StringTable *table, const char *str, size_t len);

static Identifier *insertString(
    StringTable **string_table, const char *str, size_t len);

static const char *lookupStringTable(
    const StringTable *table, const char *str, size_t len);

static void advanceInputIterator();
static char getNextCharacter();
static char peekCharacter();
static TokenType scanWithTable(char c);
