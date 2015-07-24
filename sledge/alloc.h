#ifndef MINILISP_ALLOC_H
#define MINILISP_ALLOC_H

#include "minilisp.h"
#include <string.h>
#include <stdio.h>
#include "strmap.h"

#define env_t StrMap

#ifdef CPU_ARM
#define STACK_FRAME_MARKER 0x11111111
#endif

#ifdef CPU_X64
#define STACK_FRAME_MARKER 0x1111111111111111
#endif

enum cell_allocator_t {
  CA_STACK,
  CA_HEAP
};

typedef struct MemStats {
  unsigned long byte_heap_used;
  unsigned long byte_heap_max;
  unsigned long cells_used;
  unsigned long cells_max;
} MemStats;

void init_allocator();

Cell* get_cell_heap();
void* cell_malloc(int num_bytes);
void* cell_realloc(void* old_addr, unsigned int old_size, unsigned int num_bytes);
Cell* collect_garbage(env_t* global_env, void* stack_end, void* stack_pointer);

Cell* alloc_cons(Cell* ar, Cell* dr);
Cell* alloc_list(Cell** items, int num);
Cell* alloc_sym(char* str);
Cell* alloc_bytes();
Cell* alloc_num_bytes(unsigned int num_bytes);
Cell* alloc_string();
Cell* alloc_num_string(unsigned int num_bytes);
Cell* alloc_string_copy(char* str);
Cell* alloc_concat(Cell* str1, Cell* str2);
Cell* alloc_substr(Cell* str, unsigned int from, unsigned int len);
Cell* alloc_int(int i);
Cell* alloc_nil();
Cell* alloc_error(unsigned int code);
Cell* alloc_lambda(Cell* args);
Cell* alloc_builtin(unsigned int b, Cell* signature);
Cell* alloc_clone(Cell* orig);

MemStats* alloc_stats();

void  free_tree(Cell* root);

#endif