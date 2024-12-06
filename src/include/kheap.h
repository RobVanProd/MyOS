#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

// Heap magic number for integrity checks
#define HEAP_MAGIC 0x123890AB

// Heap flags
#define HEAP_SUPERVISOR 0x1
#define HEAP_READONLY  0x2

// Memory block header structure
typedef struct block_header {
    uint32_t magic;                    // Magic number to detect corruption
    uint32_t size;                     // Size of the block (including header)
    uint8_t is_free;                   // Is this block free?
    struct block_header *next;         // Next block in the list
    struct block_header *prev;         // Previous block in the list
    uint32_t checksum;                 // Checksum for validation
} block_header_t;

// Heap structure
typedef struct heap {
    uint32_t start_address;
    uint32_t end_address;
    uint32_t max_address;
    uint32_t current_size;
    uint8_t supervisor;
    uint8_t readonly;
    block_header_t* free_list;         // List of free blocks
} heap_t;

// Initialize kernel heap
void init_kheap(void);

// Create a new heap
heap_t* create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly);

// Allocate memory
void* kmalloc(uint32_t size);

// Allocate page-aligned memory
void* kmalloc_aligned(uint32_t size);

// Allocate memory and return physical address
void* kmalloc_physical(uint32_t size, uint32_t* physical);

// Allocate page-aligned memory and return physical address
void* kmalloc_aligned_physical(uint32_t size, uint32_t* physical);

// Free memory
void kfree(void* ptr);

// Heap operations
void* heap_alloc(heap_t* heap, uint32_t size);
void heap_free(heap_t* heap, void* ptr);
uint32_t expand_heap(heap_t* heap, uint32_t size);

// Heap debugging
void get_heap_stats(uint32_t* total_blocks, uint32_t* free_blocks, uint32_t* largest_free);
void heap_dump(void);
int heap_check(void);

#endif // KHEAP_H