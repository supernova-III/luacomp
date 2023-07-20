#include <string.h>
#include "tokenizer.h"
#include "common.h"

static inline bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int64_t hexCharToNumber(char c) {
  // convert character to upper case
  if (c >= 'a') {
    c = c - ('A' - 'a');
  }
  return c - 'A' + 10;
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

static inline int64_t charToDigit(char c) {
  return c - '0';
}

static inline unsigned int hashKeyword(
    register const char *str, register size_t len) {
  static unsigned char asso_values[] = {30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 10, 10, 30,
      15, 5, 5, 15, 30, 0, 30, 10, 0, 30, 0, 10, 30, 30, 0, 30, 15, 15, 30, 0,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
      30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
  return len + asso_values[(unsigned char)str[len - 1]] +
         asso_values[(unsigned char)str[0]];
}

typedef struct {
  const char *string;
  size_t len;
  TokenType token_type;
} HashTableEntry;

#define HASH_TABLE_ENTRY(literal, type) \
  { .string = literal, .len = sizeof(literal) - 1, .token_type = type }

#define TOTAL_KEYWORDS 22
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 8
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 29
static TokenType lookupKeyword(register const char *str, register size_t len) {
  // clang-format off
  static HashTableEntry wordlist[] = {
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("in", TOKEN_IN),
    HASH_TABLE_ENTRY("nil", TOKEN_NIL),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("local", TOKEN_LOCAL),
    HASH_TABLE_ENTRY("return",TOKEN_RETURN),
    HASH_TABLE_ENTRY("if", TOKEN_IF),
    HASH_TABLE_ENTRY("for", TOKEN_FOR),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("while", TOKEN_WHILE),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("or", TOKEN_OR),
    HASH_TABLE_ENTRY("function", TOKEN_FUNCTION),
    HASH_TABLE_ENTRY("else", TOKEN_ELSE),
    HASH_TABLE_ENTRY("false", TOKEN_FALSE),
    HASH_TABLE_ENTRY("elseif",TOKEN_ELSEIF),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("not", TOKEN_NOT),
    HASH_TABLE_ENTRY("then", TOKEN_THEN),
    HASH_TABLE_ENTRY("until", TOKEN_UNTIL),
    HASH_TABLE_ENTRY("repeat", TOKEN_REPEAT),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID}, 
    HASH_TABLE_ENTRY("end", TOKEN_END),
    HASH_TABLE_ENTRY("true", TOKEN_TRUE),
    HASH_TABLE_ENTRY("break", TOKEN_BREAK),
    {.string = "", .len = 0, .token_type = TOKEN_INVALID},
    HASH_TABLE_ENTRY("do", TOKEN_DO),
    HASH_TABLE_ENTRY("and", TOKEN_AND),
    HASH_TABLE_ENTRY("goto", TOKEN_GOTO)
  };
  // clang-format on

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
    register uint8_t key = hashKeyword(str, len);

    if (key <= MAX_HASH_VALUE) {
      const HashTableEntry *s = &wordlist[key];

      if (len == s->len && !strncmp(str, s->string, len)) return s->token_type;
    }
  }
  return TOKEN_INVALID;
}

// seed may be 5381
// seed may be 5387
static inline uint32_t hashString(const char *str, size_t len, uint32_t seed) {
  uint32_t h = seed ^ (uint32_t)len;
  for (; len > 0; len--) {
    h ^= ((h << 5) + (h >> 2) + (uint8_t)str[len - 1]);
  }
  return h;
}

typedef struct {
  size_t size;
  size_t capacity;
  uint8_t memory[];
} MemoryPool;

static inline size_t computeMemoryPoolSpace(size_t capacity) {
  return sizeof(MemoryPool) + capacity;
}

static MemoryPool *allocateMemoryPool(size_t capacity) {
  MemoryPool *pool = (MemoryPool *)Calloc(1, computeMemoryPoolSpace(capacity));
  if (pool) {
    pool->capacity = capacity;
  }
  return pool;
}

static uint8_t *allocateFromMemoryPool(MemoryPool **pool, size_t n) {
  const size_t new_size = (*pool)->size + n;
  if (new_size > (*pool)->capacity) {
    const size_t new_capacity = computeMemoryPoolSpace(new_size);
    MemoryPool *new_pool = Realloc(*pool, new_capacity);
    if (new_pool) {
      *pool = new_pool;
    } else {
      return NULL;
    }
  }
  uint8_t *memory = (*pool)->memory + (*pool)->size;
  (*pool)->size += n;
  return memory;
}

typedef StringView Identifier;

static MemoryPool *identifier_memory_pool = NULL;

static Identifier *allocateIdentifier(const char *str, size_t len) {
  Identifier *res = (Identifier *)allocateFromMemoryPool(
      &identifier_memory_pool, len + 1 + sizeof(Identifier));
  if (res) {
    res->len = len;
    memcpy(res->str, str, len);
  }
  return res;
}

typedef struct StringNode {
  struct StringNode *next;
  Identifier *identifier;
} StringNode;

typedef struct {
  StringNode *head;
  StringNode *tail;
} StringList;

static MemoryPool *string_nodes_pool = NULL;

static StringNode *allocateStringNode() {
  StringNode *node = (StringNode *)allocateFromMemoryPool(
      &string_nodes_pool, sizeof(StringNode));
  return node;
}

typedef struct {
  double max_load;
  size_t hash_seed;
  size_t item_size;
  size_t capacity;
  size_t n_buckets;
  StringList buckets[];
} StringTable;

static StringTable *allocateStringTable(size_t capacity) {
  const size_t item_size = sizeof(StringList);
  const size_t allocation_size = sizeof(StringTable) + item_size * capacity;
  StringTable *table = (StringTable *)Calloc(1, allocation_size);
  if (table) {
    table->max_load = 0.6;
    table->hash_seed = 5381;
    table->capacity = capacity;
    table->item_size = item_size;
  }
  return table;
}

typedef struct {
  bool collision;
  StringNode *node;
} InsertStringResult;

static InsertStringResult insertStringBasic(
    StringTable *table, const char *str, size_t len) {
  const size_t hash = hashString(str, len, table->hash_seed);
  const size_t bucket_index = hash % table->capacity;
  StringList *list = &table->buckets[bucket_index];
  InsertStringResult res = {0};
  if (!list->head) {
    list->head = allocateStringNode();
    if (list->head) {
      list->tail = list->head;
      list->head->next = list->tail;
      list->tail->next = NULL;
    }
    res.node = list->head;
  } else {
    if (len == list->head->identifier->len &&
        !strncmp(list->head->identifier->str, str, len)) {
      res.node = list->head;
      return res;
    }
    list->tail->next = allocateStringNode();
    if (list->tail->next) {
      list->tail = list->tail->next;
    }
    res.node = list->tail;
    res.collision = true;
  }
  if (res.node) {
    res.node->identifier = allocateIdentifier(str, len + 1);
    if (res.node->identifier) {
      res.node->identifier->len = len;
      memcpy(res.node->identifier->str, str, len);
    }
  }
  return res;
}

static Identifier *insertString(
    StringTable **string_table, const char *str, size_t len) {
  StringTable *table = *string_table;
  const size_t load_factor = table->n_buckets / table->capacity;
  if (load_factor >= table->max_load) {
    // the entire table must be reallocated and rehashed, unfortunately
    const size_t allocation_size =
        2 * (size_t)((double)table->n_buckets / table->max_load);
    StringTable *new_table = Calloc(1, allocation_size);
    if (new_table) {
      for (size_t i = 0; i < table->capacity; ++i) {
        StringList *list = &table->buckets[i];
        if (list) {
          StringNode *node = list->head;
          while (node) {
            const InsertStringResult res = insertStringBasic(
                new_table, node->identifier->str, node->identifier->len);
            if (!res.node) {
              return NULL;
            }
            node = node->next;
          }
        }
      }
    } else {
      return NULL;
    }
    *string_table = new_table;
  }
  table = *string_table;

  InsertStringResult res = insertStringBasic(table, str, len);
  if (res.node) {
    table->n_buckets += res.collision;
    return res.node->identifier;
  }
  return NULL;
}

static const char *lookupStringTable(
    const StringTable *table, const char *str, size_t len) {
  const size_t idx = hashString(str, len, table->hash_seed) % table->capacity;
  const StringList *list = &table->buckets[idx];
  const char *result = NULL;
  if (list->head) {
    StringNode *node = list->head;
    while (node) {
      if (len == node->identifier->len &&
          !strncmp(node->identifier->str, str, len)) {
        result = node->identifier->str;
        break;
      }
      node = node->next;
    }
  }
  return result;
}

typedef struct TokenIterator {
  const char *input;
  size_t len;
  const char *it;
  Token current;
} TokenIterator;

static TokenIterator iterator = {.current.type = TOKEN_INVALID};
static StringTable *string_table = NULL;

void InitTokenizer(const char *input, size_t len) {
  iterator.input = input;
  iterator.len = len;
  iterator.it = input;
  string_table = allocateStringTable(128 * sizeof(StringList));
  string_nodes_pool = allocateMemoryPool(128 * 6 * sizeof(StringNode));
  identifier_memory_pool = allocateMemoryPool(32 * 1024 * 1024);
  if (!string_table || !string_nodes_pool || !identifier_memory_pool) {
    printf("No enough memory\n");
  }
}

static inline void advanceInputIterator() {
  if (iterator.it - iterator.input < iterator.len) {
    ++iterator.it;
  }
}

static inline char getNextCharacter() {
  advanceInputIterator();
  return *iterator.it;
}

static inline char peekCharacter() {
  return *iterator.it;
}

const Token *PeekToken() {
  return &iterator.current;
}

static inline TokenType scanDiGraph(TokenType alt1, char next, TokenType alt2) {
  TokenType res = alt1;
  char c = getNextCharacter();
  if (c == next) {
    res = alt2;
    advanceInputIterator();
  }
  return res;
}

static inline TokenType scanDiGraph2(TokenType main_value, TokenType alt1,
    char next1, TokenType alt2, char next2) {
  TokenType res = main_value;
  char c = getNextCharacter();
  if (c == next1) {
    res = alt1;
    advanceInputIterator();
  } else if (c == next2) {
    res = alt2;
    advanceInputIterator();
  }
  return res;
}

const Token *NextToken() {
  char c = peekCharacter();

repeat:
  if (!c) {
    iterator.current.type = TOKEN_END_OF_STREAM;
    return &iterator.current;
  }

  switch (c) {
    case '"': {
    } break;
    case '\'': {
    } break;
    case '-': {
      c = getNextCharacter();
      if (c != '-') {
        iterator.current.type = TOKEN_MINUS;
      } else {
        while (c != '\n' && c != 0) {
          c = getNextCharacter();
        }
        goto repeat;
      }
    } break;
    case '/': {
      iterator.current.type = scanDiGraph(TOKEN_DIVIDE, '/', TOKEN_DIV);
    } break;
    case '~': {
      iterator.current.type = scanDiGraph(TOKEN_BNOT, '/', TOKEN_BNOT_ASSIGN);
    } break;
    case '<': {
      iterator.current.type =
          scanDiGraph2(TOKEN_LESS, TOKEN_BLEFT, '<', TOKEN_LESS_EQUAL, '=');
    } break;
    case '>': {
      iterator.current.type = scanDiGraph(TOKEN_BIGGER, '>', TOKEN_BRIGHT);
    } break;
    case '=': {
      iterator.current.type = scanDiGraph2(
          TOKEN_ASSIGN, TOKEN_EQUALS, '=', TOKEN_BIGGER_EQUAL, '>');

    } break;
    case ':': {
      iterator.current.type = scanDiGraph(TOKEN_COLON, ':', TOKEN_COLON_COLON);
    } break;
    case '.': {
      iterator.current.type = TOKEN_PERIOD;
      c = getNextCharacter();
      if (c == '.') {
        iterator.current.type = TOKEN_2PERIOD;
        c = getNextCharacter();
        if (c == '.') {
          iterator.current.type = TOKEN_3PERIOD;
          advanceInputIterator();
        }
      }
    } break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
    } break;
    // clang-format off
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W':
    case 'X': case 'Y': case 'Z': {
      const char *start = iterator.it;
      c = getNextCharacter();
      while(isAlpha(c) || isDigit(c) || c == '_') {
        c = getNextCharacter();
      }
      const size_t len = iterator.it - start;
      TokenType token_type = lookupKeyword(start, len);
      if (token_type != TOKEN_INVALID) {
        iterator.current.type = token_type;
      } else {
        iterator.current.type = TOKEN_IDENTIFIER;
        Identifier *identifier = insertString(&string_table, start, len);
        if (!identifier) {
          printf("Cannot store identifier\n");
        }
        iterator.current.value.identifier = identifier;
      }
      // clang-format on
    } break;
    case ' ':
    case '\t':
    case '\n': {
      c = getNextCharacter();
      goto repeat;
    } break;
    default: {
      iterator.current.type = c;
      advanceInputIterator();
    }
  }
  return &iterator.current;
}
