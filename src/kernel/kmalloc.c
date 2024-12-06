#include "kmalloc.h"
#include "memory.h"

// Simple memory block header
typedef struct block_header {
    size_t size;
    uint8_t is_free;
    struct block_header* next;
} block_header_t;

static block_header_t* heap_start = NULL;

// Initialize kernel heap
static void init_heap(void) {
    if (heap_start) return;  // Already initialized

    // Request initial memory from the physical memory manager
    heap_start = (block_header_t*)memory_alloc(4096);  // Start with 4KB
    if (heap_start) {
        heap_start->size = 4096 - sizeof(block_header_t);
        heap_start->is_free = 1;
        heap_start->next = NULL;
    }
}

void* kmalloc(size_t size) {
    if (!heap_start) init_heap();
    if (!heap_start) return NULL;  // Initialization failed

    // Find a free block that's big enough
    block_header_t* current = heap_start;
    while (current) {
        if (current->is_free && current->size >= size) {
            // Found a suitable block
            if (current->size > size + sizeof(block_header_t) + 16) {
                // Split the block if it's much larger than needed
                block_header_t* new_block = (block_header_t*)((uint8_t*)current + sizeof(block_header_t) + size);
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            return (void*)((uint8_t*)current + sizeof(block_header_t));
        }
        current = current->next;
    }

    // No suitable block found, try to extend heap
    size_t alloc_size = 4096;
    while (alloc_size < size + sizeof(block_header_t))
        alloc_size *= 2;

    block_header_t* new_block = (block_header_t*)memory_alloc(alloc_size);
    if (!new_block) return NULL;

    new_block->size = alloc_size - sizeof(block_header_t);
    new_block->is_free = 0;
    new_block->next = NULL;

    // Add to end of heap
    current = heap_start;
    while (current->next)
        current = current->next;
    current->next = new_block;

    return (void*)((uint8_t*)new_block + sizeof(block_header_t));
}

void kfree(void* ptr) {
    if (!ptr) return;

    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    header->is_free = 1;

    // Merge with next block if it's free
    if (header->next && header->next->is_free) {
        header->size += sizeof(block_header_t) + header->next->size;
        header->next = header->next->next;
    }

    // Find previous block to merge if it's free
    block_header_t* current = heap_start;
    while (current && current->next != header)
        current = current->next;

    if (current && current->is_free) {
        current->size += sizeof(block_header_t) + header->size;
        current->next = header->next;
    }
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (!size) {
        kfree(ptr);
        return NULL;
    }

    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    if (header->size >= size) return ptr;  // Current block is big enough

    // Allocate new block
    void* new_ptr = kmalloc(size);
    if (!new_ptr) return NULL;

    // Copy old data
    for (size_t i = 0; i < header->size; i++)
        ((uint8_t*)new_ptr)[i] = ((uint8_t*)ptr)[i];

    kfree(ptr);
    return new_ptr;
}
