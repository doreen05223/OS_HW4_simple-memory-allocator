#ifndef HW_MALLOC_H
#define HW_MALLOC_H
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define chunk_size_t long long int
#define chunk_flag_t int

struct chunk_header {
    struct chunk_header* prev;
    struct chunk_header* next;
    chunk_size_t prev_chunk_size;
    chunk_size_t current_chunk_size;
    chunk_flag_t allocated_flag;
    chunk_flag_t mmap_bit;
    chunk_flag_t prev_free_flag;
};
struct heap_t {
    struct chunk_header* BIN[11];
    void* start_brk;
    struct heap_t* next;
};
struct heap_t *Heap;
struct chunk_header* address;

void *hw_malloc(size_t bytes);
int hw_free(void *mem);
void *get_start_sbrk(void);
void print_bin(int index);
void print_mmap_alloc_list();

#endif
