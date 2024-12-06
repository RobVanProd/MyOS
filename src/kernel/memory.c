#include <memory.h>
#include <string.h>
#include "kheap.h"
#include "terminal.h"

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
page_t* alloc_page(void) {
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!(page_bitmap[i / 32] & (1 << (i % 32)))) {
            page_bitmap[i / 32] |= (1 << (i % 32));
            free_pages--;
            page_t* page = kmalloc(sizeof(page_t));
            if (page) {
                page->present = 1;
                page->frame = i;
                return page;
            }
            // If kmalloc fails, free the frame
            page_bitmap[i / 32] &= ~(1 << (i % 32));
            free_pages++;
            return NULL;
        }
    }
    return NULL;
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

// Page directory management
static page_directory_t* kernel_directory = NULL;

page_directory_t* create_page_directory(void) {
    page_directory_t* dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!dir) return NULL;
    
    memset(dir, 0, sizeof(page_directory_t));
    
    // Copy kernel page tables
    for (int i = 768; i < 1024; i++) {
        dir->tables[i] = kernel_directory->tables[i];
        dir->tables_physical[i] = kernel_directory->tables_physical[i];
    }
    
    return dir;
}

page_directory_t* copy_page_directory(page_directory_t* src) {
    page_directory_t* dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!dir) return NULL;
    
    // Copy the page directory structure
    memcpy(dir, src, sizeof(page_directory_t));
    
    // Allocate and copy page tables
    for (int i = 0; i < 768; i++) {  // Only copy user space
        if (src->tables[i]) {
            page_table_t* table = (page_table_t*)kmalloc(sizeof(page_table_t));
            if (!table) {
                free_page_directory(dir);
                return NULL;
            }
            memcpy(table, src->tables[i], sizeof(page_table_t));
            dir->tables[i] = table;
            dir->tables_physical[i] = (uint32_t)table | 0x7;  // Present, RW, User
        }
    }
    
    return dir;
}

void free_page_directory(page_directory_t* dir) {
    if (!dir) return;
    
    // Free page tables (only user space)
    for (int i = 0; i < 768; i++) {
        if (dir->tables[i]) {
            kfree(dir->tables[i]);
        }
    }
    
    kfree(dir);
}

page_directory_t* get_kernel_page_directory(void) {
    return kernel_directory;
}

void switch_page_directory(page_directory_t* dir) {
    if (!dir) return;
    
    uint32_t cr3 = dir->physical_addr;
    asm volatile("mov %0, %%cr3" :: "r"(cr3));
}

// Page and region management
void allocate_page(page_t* page, int is_kernel, int is_writeable) {
    if (page->frame != 0) return;  // Page already allocated
    
    void* frame = alloc_page();
    if (!frame) {
        kprintf("Failed to allocate physical frame!\n");
        return;
    }
    
    page->present = 1;
    page->rw = (is_writeable) ? 1 : 0;
    page->user = (is_kernel) ? 0 : 1;
    page->frame = (uint32_t)frame / 4096;
}

void free_page(page_t* page) {
    if (!page) return;
    uint32_t frame = page->frame;
    if (frame) {
        page_bitmap[frame / 32] &= ~(1 << (frame % 32));
        page->frame = 0;
        page->present = 0;
        free_pages++;
    }
}

bool allocate_region(page_directory_t* dir, uint32_t start, uint32_t size, uint32_t flags) {
    uint32_t start_page = start / 4096;
    uint32_t end_page = (start + size - 1) / 4096;
    
    for (uint32_t page = start_page; page <= end_page; page++) {
        uint32_t table_idx = page / 1024;
        uint32_t page_idx = page % 1024;
        
        if (!dir->tables[table_idx]) {
            // Create new page table
            page_table_t* table = (page_table_t*)kmalloc(sizeof(page_table_t));
            if (!table) return false;
            
            memset(table, 0, sizeof(page_table_t));
            dir->tables[table_idx] = table;
            dir->tables_physical[table_idx] = (uint32_t)table | (flags & 0x7);
        }
        
        page_t* p = &dir->tables[table_idx]->pages[page_idx];
        allocate_page(p, !(flags & PAGE_USER), flags & PAGE_WRITE);
    }
    
    return true;
}

void free_region(page_directory_t* dir, uint32_t start, uint32_t size) {
    uint32_t start_page = start / 4096;
    uint32_t end_page = (start + size - 1) / 4096;
    
    for (uint32_t page = start_page; page <= end_page; page++) {
        uint32_t table_idx = page / 1024;
        uint32_t page_idx = page % 1024;
        
        if (dir->tables[table_idx]) {
            free_page(&dir->tables[table_idx]->pages[page_idx]);
        }
    }
}
