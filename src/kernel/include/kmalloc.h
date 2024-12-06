#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

// Memory allocation functions
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);

#endif // KMALLOC_H
