#include <memory.h>
#include <string.h>

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
    return free_pages * 4096;
}

size_t get_used_memory(void) {
    return get_total_memory() - get_free_memory();
}

// Simple memory allocation
static void* heap_start = (void*)0x200000;  // Start heap at 2MB
static void* heap_end = (void*)0x400000;    // End heap at 4MB
static void* next_free = (void*)0x200000;

void* kmalloc(size_t size) {
    // Align size to 4 bytes
    size = (size + 3) & ~3;
    
    if (next_free + size > heap_end) {
        return NULL;
    }
    
    void* ptr = next_free;
    next_free += size;
    return ptr;
}

void kfree(void* ptr) {
    // Simple allocator - no real free for now
    (void)ptr;
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) {
        return kmalloc(size);
    }
    
    void* new_ptr = kmalloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    memcpy(new_ptr, ptr, size);
    kfree(ptr);
    return new_ptr;
}

// Memory mapping
void* mmap(void* addr, size_t length, int prot, int flags) {
    (void)addr;
    (void)prot;
    (void)flags;
    return kmalloc(length);
}

int munmap(void* addr, size_t length) {
    (void)length;
    kfree(addr);
    return 0;
}
