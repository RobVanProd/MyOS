#include "kheap.h"
#include "memory.h"
#include "paging.h"
#include "terminal.h"
#include <memory.h>
#include <string.h>
#include <stdint.h>

// Minimum block size (including header)
#define MIN_BLOCK_SIZE 32

// Block header structure
typedef struct block_header {
    uint32_t magic;
    uint32_t size;
    uint8_t is_free;
    struct block_header* next;
    struct block_header* prev;
    uint32_t checksum;
} block_header_t;

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

// Kernel heap
static heap_t* kheap = NULL;

// Calculate checksum for a block header
static uint32_t calculate_checksum(block_header_t* header) {
    uint32_t sum = 0;
    uint32_t* ptr = (uint32_t*)header;
    for (size_t i = 0; i < (sizeof(block_header_t) - sizeof(uint32_t)) / 4; i++) {
        sum += ptr[i];
    }
    return sum;
}

// Update checksum for a block header
static void update_checksum(block_header_t* header) {
    header->checksum = calculate_checksum(header);
}

// Validate block header
static int validate_header(block_header_t* header) {
    if (!header) return 0;
    if (header->magic != HEAP_MAGIC) return 0;
    if (calculate_checksum(header) != header->checksum) return 0;
    return 1;
}

// Initialize kernel heap
void init_kheap(void) {
    // Start with a small heap (16MB)
    uint32_t heap_start = 0xC0400000;
    uint32_t heap_end = heap_start + 0x1000000;
    uint32_t heap_max = heap_start + 0x10000000;
    
    // Map initial heap pages
    for (uint32_t addr = heap_start; addr < heap_end; addr += PAGE_SIZE) {
        uint32_t flags = PAGE_PRESENT | PAGE_WRITE;
        map_page(addr, find_free_frame(), flags);
    }
    
    kheap = create_heap(heap_start, heap_end, heap_max, 1, 0);
}

// Allocate memory
void* kmalloc(uint32_t size) {
    if (!kheap) {
        // Early allocation before heap is initialized
        uint32_t addr = placement_address;
        placement_address += size;
        return (void*)addr;
    }
    
    return heap_alloc(kheap, size);
}

// Allocate aligned memory
void* kmalloc_aligned(uint32_t size) {
    if (!kheap) {
        uint32_t addr = placement_address;
        if (addr & 0xFFFFF000) {
            addr &= 0xFFFFF000;
            addr += 0x1000;
        }
        placement_address = addr + size;
        return (void*)addr;
    }
    
    // Round up size to page boundary
    size = (size + 0xFFF) & 0xFFFFF000;
    return heap_alloc(kheap, size);
}

// Allocate physical memory
void* kmalloc_physical(uint32_t size, uint32_t* physical) {
    void* addr = kmalloc(size);
    if (physical) {
        *physical = (uint32_t)addr - KERNEL_VIRTUAL_BASE;
    }
    return addr;
}

// Allocate aligned physical memory
void* kmalloc_aligned_physical(uint32_t size, uint32_t* physical) {
    void* addr = kmalloc_aligned(size);
    if (physical) {
        *physical = (uint32_t)addr - KERNEL_VIRTUAL_BASE;
    }
    return addr;
}

// Free memory
void kfree(void* ptr) {
    if (!ptr) return;
    heap_free(kheap, ptr);
}

// Create a new heap
heap_t* create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly) {
    heap_t* heap = (heap_t*)kmalloc(sizeof(heap_t));
    if (!heap) return NULL;
    
    heap->start_address = start;
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;
    heap->current_size = 0;
    
    return heap;
}

// Allocate memory from heap
void* heap_alloc(heap_t* heap, uint32_t size) {
    if (!heap || size == 0) return NULL;
    
    // Round up to nearest page size
    size = (size + 0xFFF) & 0xFFFFF000;
    
    uint32_t addr = heap->start_address + heap->current_size;
    if (addr + size > heap->max_address) return NULL;
    
    // Map pages
    for (uint32_t i = 0; i < size; i += 0x1000) {
        uint32_t flags = PAGE_PRESENT | PAGE_WRITE;
        if (!heap->supervisor) flags |= PAGE_USER;
        map_page(addr + i, find_free_frame(), flags);
    }
    
    heap->current_size += size;
    return (void*)addr;
}

// Free memory in heap
void heap_free(heap_t* heap, void* ptr) {
    if (!heap || !ptr) return;
    
    uint32_t addr = (uint32_t)ptr;
    if (addr < heap->start_address || addr >= heap->end_address) return;
    
    // Unmap pages
    uint32_t size = 0x1000;  // Always free at least one page
    while (addr + size <= heap->end_address) {
        unmap_page(addr + size - 0x1000);
        size += 0x1000;
    }
    
    // Update heap size
    if (addr + size == heap->start_address + heap->current_size) {
        heap->current_size -= size;
    }
}

typedef struct heap {
    uint32_t start_address;
    uint32_t end_address;
    uint32_t max_address;
    uint32_t current_size;
    uint8_t supervisor;
    uint8_t readonly;
} heap_t;

// Find the best fitting block for a given size
static block_header_t *find_best_fit(heap_t *heap, uint32_t size) {
    block_header_t *best_fit = NULL;
    uint32_t best_size = 0xFFFFFFFF;
    
    block_header_t *block = heap->free_list;
    while (block) {
        if (!validate_header(block)) {
            terminal_writestring("Heap corruption detected!\n");
            return NULL;
        }
        
        if (block->is_free && block->size >= size) {
            if (block->size < best_size) {
                best_size = block->size;
                best_fit = block;
            }
        }
        block = block->next;
    }
    
    return best_fit;
}

// Split a block if it's too large
static void split_block(block_header_t *block, uint32_t size) {
    if (block->size >= size + sizeof(block_header_t) + MIN_BLOCK_SIZE) {
        block_header_t *new_block = (block_header_t*)((uint32_t)block + size);
        new_block->magic = HEAP_MAGIC;
        new_block->size = block->size - size;
        new_block->is_free = 1;
        new_block->next = block->next;
        new_block->prev = block;
        update_checksum(new_block);
        
        if (block->next) {
            block->next->prev = new_block;
            update_checksum(block->next);
        }
        
        block->size = size;
        block->next = new_block;
        update_checksum(block);
    }
}

// Merge adjacent free blocks
static void coalesce_blocks(heap_t *heap) {
    block_header_t *block = heap->free_list;
    while (block && block->next) {
        if (!validate_header(block) || !validate_header(block->next)) {
            terminal_writestring("Heap corruption detected during coalescing!\n");
            return;
        }
        
        if (block->is_free && block->next->is_free) {
            block->size += block->next->size;
            block->next = block->next->next;
            if (block->next) {
                block->next->prev = block;
                update_checksum(block->next);
            }
            update_checksum(block);
        } else {
            block = block->next;
        }
    }
}

// Expand the heap
uint32_t expand_heap(heap_t *heap, uint32_t size) {
    if (heap->end_address + size > heap->max_address) {
        return 0;
    }
    
    uint32_t old_end = heap->end_address;
    heap->end_address += size;
    
    // Map new pages
    for (uint32_t addr = old_end; addr < heap->end_address; addr += PAGE_SIZE) {
        uint32_t flags = PAGE_PRESENT | PAGE_WRITE;
        map_page(addr, find_free_frame(), flags);
    }
    
    // Create new free block
    block_header_t *new_block = (block_header_t*)old_end;
    new_block->magic = HEAP_MAGIC;
    new_block->size = size;
    new_block->is_free = 1;
    new_block->next = NULL;
    new_block->prev = NULL;
    update_checksum(new_block);
    
    // Add to free list
    block_header_t *last_block = heap->free_list;
    while (last_block->next) {
        last_block = last_block->next;
    }
    last_block->next = new_block;
    new_block->prev = last_block;
    update_checksum(last_block);
    
    coalesce_blocks(heap);
    return 1;
}

void get_heap_stats(uint32_t *total_blocks, uint32_t *free_blocks, uint32_t *largest_free) {
    *total_blocks = 0;
    *free_blocks = 0;
    *largest_free = 0;
    
    block_header_t *block = kheap->free_list;
    while (block) {
        if (!validate_header(block)) {
            terminal_writestring("Heap corruption detected during stats collection!\n");
            return;
        }
        
        (*total_blocks)++;
        if (block->is_free) {
            (*free_blocks)++;
            if (block->size > *largest_free) {
                *largest_free = block->size;
            }
        }
        block = block->next;
    }
}

void heap_dump(void) {
    terminal_writestring("\nHeap Dump:\n");
    terminal_writestring("----------\n");
    
    block_header_t *block = kheap->free_list;
    while (block) {
        if (!validate_header(block)) {
            terminal_writestring("Heap corruption detected during dump!\n");
            return;
        }
        
        // Convert address to hex string
        char addr[17];
        uint32_t temp = (uint32_t)block;
        for (int i = 0; i < 8; i++) {
            int digit = (temp >> ((7-i) * 4)) & 0xF;
            addr[i] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
        }
        addr[8] = '\0';
        
        terminal_writestring("Block at 0x");
        terminal_writestring(addr);
        terminal_writestring(": size=");
        
        // Convert size to decimal string
        char size[11];
        temp = block->size;
        int idx = 0;
        do {
            size[idx++] = '0' + (temp % 10);
            temp /= 10;
        } while (temp > 0);
        size[idx] = '\0';
        
        // Print in reverse
        while (--idx >= 0) {
            terminal_putchar(size[idx]);
        }
        
        terminal_writestring(block->is_free ? " (free)\n" : " (used)\n");
        block = block->next;
    }
}

int heap_check(void) {
    block_header_t *block = kheap->free_list;
    while (block) {
        if (!validate_header(block)) {
            return 0;
        }
        block = block->next;
    }
    return 1;
}