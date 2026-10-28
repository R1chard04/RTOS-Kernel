#ifndef K_MEM_H
#define K_MEM_H
#include <stddef.h>

extern const uint32_t _img_end;
extern const uint32_t _estack;
extern const uint32_t _Min_Stack_Size;

extern uint32_t heap_start;
extern uint32_t heap_end;
extern uint32_t heap_size;

int k_mem_init();
void* k_mem_alloc(size_t size);
int k_mem_dealloc(void* ptr);
int k_mem_count_extfrag(size_t size);

#endif
