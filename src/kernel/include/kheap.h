#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stdbool.h>

// Heap constants
#define KHEAP_START         0xC0000000
#define KHEAP_INITIAL_SIZE  0x100000
#define HEAP_INDEX_SIZE     0x20000
#define HEAP_MAGIC          0x123890AB
#define HEAP_MIN_SIZE       0x70000

// Block header structure
typedef struct header_t {
    uint32_t magic;     // Magic number, used for error checking and identification
    uint32_t size;      // Size of the block (including header)
    bool is_free;       // 1 if this is a hole, 0 if this is a block
    struct header_t* next;  // Next block in the list
    struct header_t* prev;  // Previous block in the list
    uint32_t checksum;  // Checksum for validation
} header_t;

// Heap structure
typedef struct {
    uint32_t start_address;      // The start of our allocated space
    uint32_t end_address;        // The end of our allocated space
    uint32_t max_address;        // The maximum address the heap can be expanded to
    uint32_t current_size;       // Current size of the heap
    bool supervisor;             // Should extra pages requested by us be mapped as supervisor-only?
    bool readonly;               // Should extra pages requested by us be mapped as read-only?
    header_t* free_list;         // List of free blocks
} heap_t;

// Function declarations
heap_t* create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly);
void* heap_alloc(heap_t* heap, uint32_t size);
void heap_free(heap_t* heap, void* p);
uint32_t expand_heap(heap_t* heap, uint32_t size);

// Memory allocation functions
void kheap_init(void);
void* kmalloc(uint32_t size);
void* kmalloc_aligned(uint32_t size);
void* kmalloc_physical(uint32_t size, uint32_t* phys);
void* kmalloc_aligned_physical(uint32_t size, uint32_t* phys);
void kfree(void* p);

// Debug functions
void heap_dump(void);
bool heap_check(void);
void get_heap_stats(uint32_t* total, uint32_t* used, uint32_t* largest_free);

#endif // KHEAP_H
