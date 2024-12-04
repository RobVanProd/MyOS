#include "kheap.h"
#include "paging.h"
#include "terminal.h"

// End of kernel's code/data - defined in linker script
extern uint32_t end;
uint32_t placement_address = (uint32_t)&end;

// Kernel heap
static heap_t *kheap = NULL;

// Calculate block header checksum
static uint32_t calculate_checksum(block_header_t *header) {
    return header->magic ^ header->size ^ (uint32_t)header->next ^ (uint32_t)header->prev;
}

// Validate block header
static int validate_header(block_header_t *header) {
    if (header->magic != HEAP_MAGIC) return 0;
    if (header->checksum != calculate_checksum(header)) return 0;
    return 1;
}

// Update block header checksum
static void update_checksum(block_header_t *header) {
    header->checksum = calculate_checksum(header);
}

// Create a new heap
heap_t *create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly) {
    heap_t *heap = (heap_t*)kmalloc(sizeof(heap_t));
    
    // Align start and end addresses
    start = (start + 0xFFF) & ~0xFFF;
    end = end & ~0xFFF;
    
    heap->start_address = start;
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;
    
    // Create initial free block
    block_header_t *initial_block = (block_header_t*)start;
    initial_block->magic = HEAP_MAGIC;
    initial_block->size = end - start;
    initial_block->is_free = 1;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    update_checksum(initial_block);
    
    heap->free_list = initial_block;
    
    return heap;
}

// Initialize kernel heap
void init_kheap(void) {
    // Create kernel heap starting at 3GB
    uint32_t heap_start = 0xC0000000;
    uint32_t heap_end = heap_start + 0x400000; // Initial size: 4MB
    uint32_t heap_max = heap_start + 0x1000000; // Max size: 16MB
    
    // Map initial heap pages
    for (uint32_t addr = heap_start; addr < heap_end; addr += PAGE_SIZE) {
        map_page(addr, find_free_frame(), 1, 1);
    }
    
    kheap = create_heap(heap_start, heap_end, heap_max, 1, 0);
}

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
        map_page(addr, find_free_frame(), heap->supervisor, !heap->readonly);
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

void *kmalloc(uint32_t size) {
    if (!kheap) {
        // If heap isn't initialized, use placement allocator
        return (void*)placement_address;
    }
    
    // Adjust size to include header and align to 8 bytes
    size = ((size + sizeof(block_header_t) + 7) & ~7);
    
    block_header_t *block = find_best_fit(kheap, size);
    if (!block) {
        // No suitable block found, try to expand heap
        if (!expand_heap(kheap, size > PAGE_SIZE ? size : PAGE_SIZE)) {
            return NULL;
        }
        block = find_best_fit(kheap, size);
        if (!block) return NULL;
    }
    
    split_block(block, size);
    block->is_free = 0;
    update_checksum(block);
    
    return (void*)((uint32_t)block + sizeof(block_header_t));
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    block_header_t *block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    if (!validate_header(block)) {
        terminal_writestring("Attempt to free invalid block!\n");
        return;
    }
    
    block->is_free = 1;
    update_checksum(block);
    coalesce_blocks(kheap);
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