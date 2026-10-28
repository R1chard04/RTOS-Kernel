#include "k_task.h"
#include "k_mem.h"
#include "common.h"

uint32_t heap_start;
uint32_t heap_end;
uint32_t heap_size;

const uint32_t free_list_node_size = sizeof(free_list_node);

free_list_node *head;
bool mem_init = false;

int k_mem_init() {
  heap_start = ((uint32_t)&_img_end + 3) & ~3;
  heap_end = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
  heap_end &= ~3;
  heap_size = heap_end - heap_start;

  head = (free_list_node *)heap_start;
  head->size = 0;
  head->free = false;
  head->owner = KERNEL_ID;

  free_list_node *first_free = (free_list_node *)(heap_start + free_list_node_size);
  head->next = first_free;

  first_free->size = heap_size - 2 * free_list_node_size;
  first_free->owner = KERNEL_ID;
  first_free->free = true;
  first_free->next = NULL;

  mem_init = true;
  return 0;
}

void* k_mem_alloc(size_t size) {
  if (!mem_init || size == 0) return NULL;

  size = (size + 3) & ~3;

  free_list_node *prev = head, *curr = head->next;

  while (curr && curr->size < size) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr) return NULL;


  if (curr->size >= size + free_list_node_size + 4) {
    free_list_node *new_block = (free_list_node *)((char *)curr + free_list_node_size + size);
    new_block->size = curr->size - size - free_list_node_size;
    new_block->owner = KERNEL_ID;
    new_block->free = true;
    new_block->next = curr->next;

    prev->next = new_block;
    curr->size = size;
  } else {
    prev->next = curr->next;
  }

  curr->owner = osGetTID();
  curr->free = false;

  return (char *)curr + free_list_node_size;
}

int k_mem_dealloc(void* ptr) {
  if (ptr == NULL) return RTX_OK;

  free_list_node *curr = (free_list_node *)((char *)ptr - free_list_node_size);

  if (curr->owner != osGetTID() || curr->free)
    return RTX_ERR;

  curr->free = true;
  curr->owner = KERNEL_ID;

  free_list_node* prev = head;
  while (prev->next && prev->next < curr) {
    prev = prev->next;
  }
  curr->next = prev->next;
  prev->next = curr;

  if (prev != head && ((char *)prev + free_list_node_size + prev->size == (char *)curr)) {
    prev->size += free_list_node_size + curr->size;
    prev->next = curr->next;
    curr = prev;
  }

  if (curr->next && ((char *)curr + free_list_node_size + curr->size == (char *)curr->next)) {
    curr->size += free_list_node_size + curr->next->size;
    curr->next = curr->next->next;
  }

  return RTX_OK;
}

int k_mem_count_extfrag(size_t size) {
  int count = 0;

  free_list_node *curr = head->next;
  while (curr) {
    if (curr->size < size) ++count;
    curr = curr->next;
  }
  return count;
}
