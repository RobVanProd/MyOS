#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>

// Memory management functions
void memory_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);

// Memory mapping function
void *mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset);
int munmap(void* addr, size_t length);

// Page allocation
void* alloc_page(void);
void free_page(void* page);

// Memory information
size_t get_total_memory(void);
size_t get_free_memory(void);
size_t get_used_memory(void);

#endif
