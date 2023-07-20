#include "common.h"
#include <stdlib.h>
#include <stdio.h>

void *Malloc(size_t size) {
  void *result = malloc(size);
  if (!result) {
    printf("Malloc failed to allocate %llu bytes\n", size);
  }
  return result;
}

void *Realloc(void *ptr, size_t size) {
  void *result = realloc(ptr, size);
  if (!result) {
    printf("Realloc failed to allocate %llu bytes\n", size);
  }
  return result;
}

void *Calloc(size_t n, size_t size) {
  void *result = calloc(n, size);
  if (!result) {
    printf("Calloc failed to allocate %llu bytes\n", size);
  }
  return result;
}

List InitList(const void *data, AllocateListNodeFunction *node_allocator) {
  List result = {.node_allocator = node_allocator};
  if (!node_allocator) {
    return result;
  }
  ListNode *node = node_allocator(data);
  result.head = node;
  result.tail = node;
  result.tail->next = NULL;
  result.head->next = result.tail;
  return result;
}

ListNode *PushToList(List *list, const void *data) {
  if (!list) return NULL;

  ListNode *node = list->node_allocator(data);
  if (!node) return NULL;
  list->tail->next = node;
  list->tail = node;
  return node;
}
