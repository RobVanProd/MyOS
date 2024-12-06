#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

// Heap magic number for integrity checks
#define HEAP_MAGIC 0x123890AB

// Heap flags
#define HEAP_SUPERVISOR 0x1
#define HEAP_READONLY  0x2

// Heap structure
typedef struct {
    uint32_t start_address;
    uint32_t end_address;
    uint32_t max_address;
    uint32_t current_size;
    uint8_t supervisor;
    uint8_t readonly;
} heap_t;

// Memory block header structure
typedef struct block_header {
    uint32_t magic;                    // Magic number to detect corruption
    uint32_t size;                     // Size of the block (including header)
    uint8_t is_free;                   // Is this block free?
    struct block_header *next;         // Next block in the list
    struct block_header *prev;         // Previous block in the list
    uint32_t checksum;                 // Checksum for validation
} block_header_t;

// Initialize kernel heap
void init_kheap(void);

// Create a new heap
heap_t *create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly);

// Allocate memory
void *kmalloc(uint32_t size);

// Allocate page-aligned memory
void *kmalloc_aligned(uint32_t size);

// Allocate memory and return physical address
void *kmalloc_physical(uint32_t size, uint32_t *physical);

// Allocate page-aligned memory and return physical address
void *kmalloc_aligned_physical(uint32_t size, uint32_t *physical);

// Free memory
void kfree(void *ptr);

// Expand heap size
uint32_t expand_heap(heap_t *heap, uint32_t size);

// Contract heap size
uint32_t contract_heap(heap_t *heap, uint32_t size);

// Get total free memory
uint32_t get_free_memory(void);

// Get total used memory
uint32_t get_used_memory(void);

// Get heap statistics
void get_heap_stats(uint32_t *total_blocks, uint32_t *free_blocks, uint32_t *largest_free);

// Memory mapping functions
void *mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset);
int munmap(void *addr, uint32_t length);

// Swap file operations
int init_swap(const char *swap_file, uint32_t size);
int swap_out(uint32_t page_addr);
int swap_in(uint32_t page_addr, uint32_t swap_offset);

// Copy-on-write support
int mark_page_cow(uint32_t virtual_addr);
int handle_cow_fault(uint32_t fault_addr);

// Debug functions
void heap_dump(void);
int heap_check(void);

#endif