#include <string.h>
#include "tokenizer.h"
#include "common.h"
#include "tokenizer_internal.h"

typedef struct TokenIterator {
  const char *input;
  size_t len;
  const char *it;
  Token current;
} TokenIterator;

static TokenIterator iterator = {.current.type = TOKEN_INVALID};
static StringTable *string_table = NULL;
static MemoryPool *string_nodes_pool = NULL;
static MemoryPool *identifier_memory_pool = NULL;

void InitTokenizer(const char *input, size_t len) {
  iterator.input = input;
  iterator.len = len;
  iterator.it = input;
  string_table = allocateStringTable(128 * sizeof(StringList));
  string_nodes_pool = allocateMemoryPool(128 * 6 * sizeof(StringNode));
  identifier_memory_pool = allocateMemoryPool(32 * 1024 * 1024);
  if (!string_table || !string_nodes_pool || !identifier_memory_pool) {
    printf("No enough memory\n");
    exit(EXIT_FAILURE);
  }
}

const Token *PeekToken() {
  return &iterator.current;
}

const Token *NextToken() {
  char c = peekCharacter();
NextToken_repeat:
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
        goto NextToken_repeat;
      }
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
      } else if (isDigit(c)) {
        iterator.current.type = TOKEN_NUMBER;
        // here we parse numbers like .123 or .123e123
        EvaluateIntegerResult res = scanAndEvaluateInteger(10);
        c = res.c;
        double number = res.magnitude / res.power_of_base;

        if (c == 'e' || c == 'E') {
          res = evaluateExponent(10);
          c = res.c;
          number *= res.magnitude;
        }

        iterator.current.value.number = number;
      }
    } break;
    // clang-format off
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': {
      // clang-format on
      double base = 10;
      if (c == '0') {
        c = getNextCharacter();
        base += 6 * (c == 'x' || c == 'X');
      }
      EvaluateIntegerResult eval_res = scanAndEvaluateInteger(base);
      c = eval_res.c;
      double number = eval_res.magnitude * eval_res.power_of_base;
      if (c == '.') {
        c = getNextCharacter();
        if (isHexadecimal(c)) {
          eval_res = scanAndEvaluateInteger(base);
          c = eval_res.c;
          number += eval_res.magnitude / eval_res.power_of_base;
        }
      }
      if (c == 'e' || c == 'E') {
        if (base == 10) {
          eval_res = evaluateExponent(10);
          c = eval_res.c;
          number *= eval_res.magnitude;
        } else {
          printf(
              "Base 10 exponent cannot be used with with non-decimal "
              "numbers\n");
          exit(EXIT_FAILURE);
        }
      } else if (c == 'p' || c == 'P') {
        if (base == 16) {
          eval_res = evaluateExponent(2);
          c = eval_res.c;
          number *= eval_res.magnitude;
        } else {
          printf("Base 2 exponen cannot be used with non-hex numbers\n");
          exit(EXIT_FAILURE);
        }
      }
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
      // clang-format on
      const char *start = iterator.it;
      c = getNextCharacter();
      while (isAlpha(c) || isDigit(c) || c == '_') {
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
          exit(EXIT_FAILURE);
        }
        iterator.current.value.identifier = identifier;
      }
    } break;
    case ' ':
    case '\t':
    case '\n': {
      c = getNextCharacter();
      goto NextToken_repeat;
    } break;
    // clang-format off
    case '+': case '*': case '%': case '#': case '&': case '|': case '(':
    case ')': case '{': case '}': case '[': case ']': case ';': case ',': {
      // clang-format on
      iterator.current.type = c;
      advanceInputIterator();
    } break;
    // clang-format off
    case '/': case '~': case '<': case '>': case '=': case ':': {
      // clang-format on
      iterator.current.type = scanWithTable(c);
    } break;
    default: {
      printf("Unexpected character: %c\n", c);
      exit(EXIT_FAILURE);
    }
  }
  return &iterator.current;
}

static inline uint32_t hashKeyword(const char *str, size_t len) {
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

#define TOTAL_KEYWORDS 22
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 8
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 29
static TokenType lookupKeyword(const char *str, size_t len) {
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

static Identifier *allocateIdentifier(const char *str, size_t len) {
  Identifier *res = (Identifier *)allocateFromMemoryPool(
      &identifier_memory_pool, len + 1 + sizeof(Identifier));
  if (res) {
    res->len = len;
    memcpy(res->str, str, len);
  }
  return res;
}

static StringNode *allocateStringNode() {
  StringNode *node = (StringNode *)allocateFromMemoryPool(
      &string_nodes_pool, sizeof(StringNode));
  return node;
}

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

typedef struct {
  char c;
  TokenType t;
} CharTokenPair;

typedef struct {
  TokenType main_type;
  size_t size;
  CharTokenPair pairs[2];
} ScannerTableEntry;

// clang-format off
static const ScannerTableEntry scanner_table[] = {
    ['/'] = {
      .main_type = TOKEN_DIVIDE,
      .size = 1,
      .pairs = {
        [0] = {.c = '/', .t = TOKEN_DIV}
      }
    },
    ['~'] = {
      .main_type = TOKEN_BNOT,
      .size = 1,
      .pairs = {[0] = {.c = '=', .t = TOKEN_BNOT_ASSIGN}
      }
    },
    ['<'] = {
      .main_type = TOKEN_LESS,
      .size = 2,
      .pairs = {
        [0] = {.c = '<', .t = TOKEN_BLEFT},
        [1] = {.c = '=', .t = TOKEN_LESS_EQUAL}
      }
    },
    ['>'] = {
      .main_type = TOKEN_BIGGER,
      .size = 2,
      .pairs = {
        [0] = {.c = '>', .t = TOKEN_BRIGHT},
        [1] = {.c = '=', .t = TOKEN_BIGGER_EQUAL}
      }
    },
    ['='] = {
      .main_type = TOKEN_ASSIGN,
      .size = 1,
      .pairs = {
        [0] = {.c = '=', .t = TOKEN_EQUALS}
      }
    },
    [':'] = {
      .main_type = TOKEN_COLON,
      .size = 1,
      .pairs = {
        [0] = {.c = ':', .t = TOKEN_COLON_COLON}
      }
    }
};
// clang-format on

static inline TokenType scanWithTable(char c) {
  const ScannerTableEntry *entry = &scanner_table[c];
  TokenType res = c;
  if (entry->size == 0) {
    advanceInputIterator();
  } else {
    size_t i = 0;
    c = getNextCharacter();
    while (i < entry->size) {
      if (c == entry->pairs[i].c) {
        res = entry->pairs[i].t;
        break;
      }
      ++i;
    }
  }
  return res;
}

static inline size_t mapBaseToIndex(double base) {
  const size_t output = ((size_t)base - 10) / 6;
  return output;
}

typedef bool CharCheckerFuncType(char c);
typedef double CharConverterFuncType(char c);
static CharCheckerFuncType *checkers[] = {isDigit, isHexadecimal};
static CharConverterFuncType *converters[] = {charToDigit, hexToNumber};

static EvaluateIntegerResult scanAndEvaluateInteger(double base) {
  const char *start = iterator.it;
  char c = getNextCharacter();
  const size_t index = mapBaseToIndex(base);
  CharCheckerFuncType *isCharOk = checkers[index];
  CharConverterFuncType *convertChar = converters[index];
  if (!isCharOk(c)) {
    return (EvaluateIntegerResult){.c = c};
  }
  while (isCharOk(c)) {
    c = getNextCharacter();
  }
  const size_t len = iterator.it - start;
  EvaluateIntegerResult result = {0};
  double magnitude = 0;
  double power_of_base = 1;
  for (size_t i = 0; i < len; ++i) {
    magnitude += power_of_base * convertChar(start[len - i - 1]);
    power_of_base *= base;
  }
  return (EvaluateIntegerResult){
      .magnitude = magnitude, .power_of_base = power_of_base, .c = c};
}

static EvaluateIntegerResult evaluateExponent(double base) {
  char c = getNextCharacter();
  double sign = 1;
  if (c == '-') {
    sign = -1;
    c = getNextCharacter();
  }
  double exponential_part = 1;
  if (isDigit(c)) {
    EvaluateIntegerResult res = scanAndEvaluateInteger(base);
    c = res.c;
    size_t power = (size_t)(res.magnitude * res.power_of_base);
    double power_of_base = 1;
    for (size_t i = 0; i < power; ++i) {
      power_of_base *= base;
    }
    if (sign == -1) {
      exponential_part /= power_of_base;
    } else {
      exponential_part *= power_of_base;
    }
  } else {
    printf("Expected integer, found %c\n", c);
  }
  return (EvaluateIntegerResult){.magnitude = exponential_part, .c = c};
}
