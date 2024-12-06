#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Memory allocation functions
void* heap_alloc(size_t size);
void heap_free(void* ptr);
void* heap_realloc(void* ptr, size_t size);
void* heap_calloc(size_t num, size_t size);

// Heap operations
void heap_init(void);
void heap_cleanup(void);

// Debug functions
void heap_dump(void);
bool heap_check(void);
void get_heap_stats(uint32_t* total, uint32_t* used, uint32_t* largest_free);

#endif // HEAP_H
