#pragma once
#include <stddef.h>
#include <stdint.h>

void *Malloc(size_t size);
void *Realloc(void *ptr, size_t size);
void *Calloc(size_t n, size_t size);

typedef struct ListNode {
  struct ListNode *next;
  size_t size;
  uint8_t data[];
} ListNode;

typedef ListNode *AllocateListNodeFunction(const void *);
typedef struct {
  ListNode *head;
  ListNode *tail;
  AllocateListNodeFunction *node_allocator;
} List;

List InitList(const void *data, AllocateListNodeFunction *node_allocator);
ListNode *PushToList(List *list, const void *data);
