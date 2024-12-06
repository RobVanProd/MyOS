#include <memory.h>
#include <string.h>
#include "kheap.h"

// Memory map entry structure
typedef struct {
    uint32_t base;
    uint32_t length;
    uint32_t type;
} __attribute__((packed)) memory_map_entry_t;

// Memory management data
static uint32_t* page_bitmap;
static uint32_t total_pages;
static uint32_t free_pages;

// Memory initialization
void memory_init(void) {
    // Initialize memory management structures
    page_bitmap = (uint32_t*)0x100000;  // Place bitmap at 1MB
    total_pages = (get_total_memory() / 4096);
    free_pages = total_pages;
    
    // Clear bitmap
    memset(page_bitmap, 0, total_pages / 8);
}

// Page allocation
void* alloc_page(void) {
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!(page_bitmap[i / 32] & (1 << (i % 32)))) {
            page_bitmap[i / 32] |= (1 << (i % 32));
            free_pages--;
            return (void*)(i * 4096);
        }
    }
    return NULL;
}

void free_page(void* page) {
    uint32_t page_index = (uint32_t)page / 4096;
    if (page_index < total_pages) {
        page_bitmap[page_index / 32] &= ~(1 << (page_index % 32));
        free_pages++;
    }
}

// Memory information
size_t get_total_memory(void) {
    return 16 * 1024 * 1024;  // 16MB for now
}

size_t get_free_memory(void) {
    return free_pages * 4096;  // Each page is 4KB
}

size_t get_used_memory(void) {
    return (total_pages - free_pages) * 4096;
}

void* krealloc(void* ptr, size_t size) {
    if (ptr == NULL) {
        // If ptr is NULL, act like malloc
        return kmalloc(size);
    }
    if (size == 0) {
        // If size is 0, act like free
        kfree(ptr);
        return NULL;
    }
    
    // Allocate new block
    void* new_ptr = kmalloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    // Copy old data to new block
    memcpy(new_ptr, ptr, size);
    
    // Free old block
    kfree(ptr);
    
    return new_ptr;
}

// Memory mapping
void* mmap(void* addr, uint32_t length, int prot, int flags, int fd, uint32_t offset) {
    // Unused parameters
    (void)addr;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;

    // Simple implementation - just allocate pages
    uint32_t num_pages = (length + 4095) / 4096;  // Round up to nearest page
    for (uint32_t i = 0; i < num_pages; i++) {
        void* page = alloc_page();
        if (page == NULL) {
            // Handle allocation failure
            return NULL;
        }
    }
    return (void*)1;  // Return non-NULL on success
}

int munmap(void* addr, size_t length) {
    // Simple implementation - just free pages
    uint32_t num_pages = (length + 4095) / 4096;  // Round up to nearest page
    uint32_t start_page = (uint32_t)addr / 4096;
    
    for (uint32_t i = 0; i < num_pages; i++) {
        free_page((void*)((start_page + i) * 4096));
    }
    
    return 0;
}
