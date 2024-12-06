#include "kheap.h"
#include "memory.h"
#include "terminal.h"
#include <stdint.h>
#include <string.h>

// Minimum block size (including header)
#define MIN_BLOCK_SIZE 64

// Frame allocation function
static uint32_t find_free_frame(void) {
    static uint32_t frame = 0;
    // Simple frame allocation strategy - just increment
    // In a real OS, you'd want to track free frames
    return frame++;
}

// End of kernel's code/data - defined in linker script
extern uint32_t end;
uint32_t placement_address = (uint32_t)&end;

// Global kernel heap
static heap_t* kheap = NULL;

// Forward declarations
static uint32_t calculate_checksum(header_t* header);
static void update_checksum(header_t* header);
static header_t* find_best_fit(heap_t* heap, uint32_t size);
static void split_block(header_t* block, uint32_t size);
static void coalesce_blocks(heap_t* heap);

// Calculate checksum for a block header
static uint32_t calculate_checksum(header_t* header) {
    uint32_t sum = 0;
    uint8_t* ptr = (uint8_t*)header;
    
    // Skip the checksum field itself
    for (size_t i = 0; i < offsetof(header_t, checksum); i++) {
        sum += ptr[i];
    }
    
    return sum;
}

// Update checksum for a block header
static void update_checksum(header_t* header) {
    header->checksum = calculate_checksum(header);
}

// Initialize the kernel heap
void kheap_init(void) {
    if (!kheap) {
        kheap = create_heap(0x100000, 0x200000, 0x1000000, 1, 0);
        if (!kheap) {
            terminal_writestring("Failed to create kernel heap!\n");
        }
    }
}

// Allocate memory from the kernel heap
void* kmalloc(uint32_t size) {
    if (kheap) {
        return heap_alloc(kheap, size);
    }
    return NULL;
}

// Allocate aligned memory from the kernel heap
void* kmalloc_aligned(uint32_t size) {
    if (kheap) {
        uint32_t aligned_size = (size + 0xFFF) & ~0xFFF;
        return heap_alloc(kheap, aligned_size);
    }
    return NULL;
}

// Free memory back to the kernel heap
void kfree(void* ptr) {
    if (kheap && ptr) {
        heap_free(kheap, ptr);
    }
}

// Create a new heap
heap_t* create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly) {
    heap_t* heap = (heap_t*)start;
    
    // Initialize heap structure
    heap->start_address = start + sizeof(heap_t);
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;
    heap->current_size = 0;
    heap->free_list = NULL;

    // Create initial free block
    header_t* initial_block = (header_t*)heap->start_address;
    initial_block->magic = HEAP_MAGIC;
    initial_block->size = end - heap->start_address - sizeof(header_t);
    initial_block->is_free = 1;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    update_checksum(initial_block);

    // Set as first free block
    heap->free_list = initial_block;

    return heap;
}

// Find best fitting block for allocation
static header_t* find_best_fit(heap_t* heap, uint32_t size) {
    header_t* best_fit = NULL;
    uint32_t best_size = 0xFFFFFFFF;

    for (header_t* block = heap->free_list; block != NULL; block = block->next) {
        if (block->is_free && block->size >= size) {
            if (block->size < best_size) {
                best_size = block->size;
                best_fit = block;
            }
        }
    }

    return best_fit;
}

// Split a block if it's too large
static void split_block(header_t* block, uint32_t size) {
    if (block->size > size + sizeof(header_t) + MIN_BLOCK_SIZE) {  
        header_t* new_block = (header_t*)((uint32_t)block + sizeof(header_t) + size);
        new_block->magic = HEAP_MAGIC;
        new_block->size = block->size - size - sizeof(header_t);
        new_block->is_free = 1;
        new_block->next = block->next;
        new_block->prev = block;
        update_checksum(new_block);

        if (block->next) {
            block->next->prev = new_block;
        }

        block->next = new_block;
        block->size = size;
        update_checksum(block);
    }
}

// Coalesce adjacent free blocks
static void coalesce_blocks(heap_t* heap) {
    for (header_t* block = heap->free_list; block != NULL; block = block->next) {
        if (block->is_free && block->next != NULL && block->next->is_free) {
            // Merge with next block
            block->size += sizeof(header_t) + block->next->size;
            block->next = block->next->next;
            if (block->next != NULL) {
                block->next->prev = block;
                update_checksum(block->next);
            }
            update_checksum(block);
        }
    }
}

// Allocate memory from heap
void* heap_alloc(heap_t* heap, uint32_t size) {
    if (!heap || size == 0) return NULL;
    
    // Find a suitable block
    header_t* block = find_best_fit(heap, size);
    if (!block) {
        // No suitable block found, try to expand heap
        uint32_t expand_size = size + sizeof(header_t);
        if (expand_heap(heap, expand_size) == 0) {
            return NULL;  // Expansion failed
        }
        block = find_best_fit(heap, size);
        if (!block) return NULL;  // Should not happen
    }
    
    // Split block if necessary
    split_block(block, size);
    
    // Mark block as used
    block->is_free = 0;
    update_checksum(block);
    
    // Return pointer to usable memory
    return (void*)((uint32_t)block + sizeof(header_t));
}

// Free memory back to heap
void heap_free(heap_t* heap, void* ptr) {
    if (!heap || !ptr) return;
    
    // Get block header
    header_t* header = (header_t*)((uint32_t)ptr - sizeof(header_t));
    
    // Validate header
    if (header->magic != HEAP_MAGIC || header->checksum != calculate_checksum(header)) {
        kprintf("Invalid block header detected in heap_free!\n");
        return;
    }
    
    // Mark block as free
    header->is_free = 1;
    update_checksum(header);
    
    // Try to coalesce blocks
    coalesce_blocks(heap);
}

// Expand the heap
uint32_t expand_heap(heap_t* heap, uint32_t size) {
    if (!heap) {
        return 0;
    }

    // Check if we can expand
    uint32_t new_size = heap->current_size + size + sizeof(header_t);
    if (heap->end_address + new_size > heap->max_address) {
        return 0;
    }

    // Create new block at the end
    header_t* new_block = (header_t*)heap->end_address;
    new_block->magic = HEAP_MAGIC;
    new_block->size = size - sizeof(header_t);
    new_block->is_free = 1;
    new_block->next = NULL;
    new_block->prev = NULL;
    update_checksum(new_block);

    // Add to free list
    if (heap->free_list == NULL) {
        heap->free_list = new_block;
    } else {
        header_t* last = heap->free_list;
        while (last->next) {
            last = last->next;
        }
        last->next = new_block;
        new_block->prev = last;
        update_checksum(new_block);
        update_checksum(last);
    }

    // Update heap size and end address
    heap->current_size += size;
    heap->end_address += size;

    return 1;
}

// Get heap statistics
void get_heap_stats(uint32_t* total, uint32_t* used, uint32_t* largest_free) {
    if (!kheap) {
        *total = 0;
        *used = 0;
        *largest_free = 0;
        return;
    }

    *total = kheap->current_size;
    *used = 0;
    *largest_free = 0;
    uint32_t largest = 0;

    for (header_t* block = kheap->free_list; block != NULL; block = block->next) {
        // Count used space
        if (!block->is_free) {
            *used += block->size + sizeof(header_t);
        } else if (block->size > largest) {
            largest = block->size;
        }
    }

    *largest_free = largest;
}

// Dump heap information for debugging
void heap_dump(void) {
    if (!kheap) {
        terminal_writestring("Heap not initialized!\n");
        return;
    }

    terminal_writestring("Heap Information:\n");
    terminal_writestring("Start: "); terminal_writehex(kheap->start_address); terminal_writestring("\n");
    terminal_writestring("End: "); terminal_writehex(kheap->end_address); terminal_writestring("\n");
    terminal_writestring("Max: "); terminal_writehex(kheap->max_address); terminal_writestring("\n");
    terminal_writestring("Size: "); terminal_writehex(kheap->current_size); terminal_writestring("\n");

    terminal_writestring("\nBlocks:\n");
    for (header_t* block = kheap->free_list; block != NULL; block = block->next) {
        terminal_writestring("Block at "); terminal_writehex((uint32_t)block); terminal_writestring(":\n");
        terminal_writestring("  Size: "); terminal_writehex(block->size); terminal_writestring("\n");
        terminal_writestring("  Free: "); terminal_writehex(block->is_free); terminal_writestring("\n");
        terminal_writestring("  Magic: "); terminal_writehex(block->magic); terminal_writestring("\n");
        terminal_writestring("  Checksum: "); terminal_writehex(block->checksum); terminal_writestring("\n");
    }
}

// Check heap integrity
bool heap_check(void) {
    if (!kheap) {
        return false;
    }

    // Check all blocks
    for (header_t* block = kheap->free_list; block != NULL; block = block->next) {
        // Check magic number
        if (block->magic != HEAP_MAGIC) {
            terminal_writestring("Invalid magic number in block!\n");
            return false;
        }

        // Check checksum
        if (block->checksum != calculate_checksum(block)) {
            terminal_writestring("Invalid checksum in block!\n");
            return false;
        }

        // Check block links
        if (block->next != NULL && block->next->prev != block) {
            terminal_writestring("Invalid block links!\n");
            return false;
        }
    }

    return true;
}