#include "include/heap.h"
#include "include/string.h"

#define HEAP_MAGIC 0x12345678
#define MIN_BLOCK_SIZE 16
#define HEAP_INITIAL_SIZE (1024 * 1024)  // 1MB initial heap

typedef struct block_header {
    uint32_t magic;
    size_t size;
    bool is_free;
    struct block_header* next;
    struct block_header* prev;
} block_header_t;

static block_header_t* heap_start = NULL;
static size_t total_size = 0;
static size_t used_size = 0;

// Forward declarations
static block_header_t* find_free_block(size_t size);
static void split_block(block_header_t* block, size_t size);
static void merge_blocks(void);

void heap_init(void) {
    // Initial heap allocation would typically use a lower-level memory allocator
    // For now, we'll assume we have a fixed memory region
    heap_start = (block_header_t*)0x100000;  // Start at 1MB mark
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = HEAP_INITIAL_SIZE - sizeof(block_header_t);
    heap_start->is_free = true;
    heap_start->next = NULL;
    heap_start->prev = NULL;
    total_size = HEAP_INITIAL_SIZE;
    used_size = sizeof(block_header_t);
}

void* heap_alloc(size_t size) {
    if (!heap_start) heap_init();
    if (size == 0) return NULL;

    // Align size to multiple of 8 bytes
    size = (size + 7) & ~7;

    block_header_t* block = find_free_block(size);
    if (!block) return NULL;

    if (block->size > size + sizeof(block_header_t) + MIN_BLOCK_SIZE) {
        split_block(block, size);
    }

    block->is_free = false;
    used_size += block->size + sizeof(block_header_t);

    return (void*)(block + 1);
}

void heap_free(void* ptr) {
    if (!ptr) return;

    block_header_t* block = ((block_header_t*)ptr) - 1;
    if (block->magic != HEAP_MAGIC) return;  // Invalid block

    block->is_free = true;
    used_size -= block->size + sizeof(block_header_t);

    merge_blocks();
}

void* heap_realloc(void* ptr, size_t size) {
    if (!ptr) return heap_alloc(size);
    if (size == 0) {
        heap_free(ptr);
        return NULL;
    }

    block_header_t* block = ((block_header_t*)ptr) - 1;
    if (block->magic != HEAP_MAGIC) return NULL;  // Invalid block

    if (block->size >= size) {
        // Current block is big enough
        if (block->size > size + sizeof(block_header_t) + MIN_BLOCK_SIZE) {
            split_block(block, size);
        }
        return ptr;
    }

    // Need to allocate new block
    void* new_ptr = heap_alloc(size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, block->size);
    heap_free(ptr);

    return new_ptr;
}

void* heap_calloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = heap_alloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

static block_header_t* find_free_block(size_t size) {
    block_header_t* current = heap_start;
    while (current) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void split_block(block_header_t* block, size_t size) {
    block_header_t* new_block = (block_header_t*)((char*)(block + 1) + size);
    new_block->magic = HEAP_MAGIC;
    new_block->size = block->size - size - sizeof(block_header_t);
    new_block->is_free = true;
    new_block->next = block->next;
    new_block->prev = block;

    block->size = size;
    block->next = new_block;

    if (new_block->next) {
        new_block->next->prev = new_block;
    }
}

static void merge_blocks(void) {
    block_header_t* current = heap_start;
    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(block_header_t) + current->next->size;
            current->next = current->next->next;
            if (current->next) {
                current->next->prev = current;
            }
        } else {
            current = current->next;
        }
    }
}

void heap_dump(void) {
    block_header_t* current = heap_start;
    while (current) {
        // Add debug output here if needed
        current = current->next;
    }
}

bool heap_check(void) {
    block_header_t* current = heap_start;
    while (current) {
        if (current->magic != HEAP_MAGIC) return false;
        if (current->next && current->next->prev != current) return false;
        current = current->next;
    }
    return true;
}

void get_heap_stats(uint32_t* total, uint32_t* used, uint32_t* largest_free) {
    if (total) *total = total_size;
    if (used) *used = used_size;
    
    if (largest_free) {
        size_t max_free = 0;
        block_header_t* current = heap_start;
        while (current) {
            if (current->is_free && current->size > max_free) {
                max_free = current->size;
            }
            current = current->next;
        }
        *largest_free = max_free;
    }
}
