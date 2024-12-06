#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include "kheap.h"

// Page size
#define PAGE_SIZE 4096

// Page flags
#define PAGE_PRESENT  0x1
#define PAGE_WRITE    0x2
#define PAGE_USER     0x4
#define PAGE_ACCESSED 0x20
#define PAGE_DIRTY    0x40

// Page directory and table structures
typedef struct page {
    uint32_t present    : 1;   // Page present in memory
    uint32_t rw         : 1;   // Read-only if clear, readwrite if set
    uint32_t user       : 1;   // Supervisor level only if clear
    uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;   // Has the page been written to since last refresh?
    uint32_t unused     : 7;   // Amalgamation of unused and reserved bits
    uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table {
    page_t pages[1024];
} page_table_t;

typedef struct page_directory {
    page_table_t* tables[1024];    // Array of pointers to page tables
    uint32_t tables_physical[1024]; // Array of physical addresses of page tables
    uint32_t physical_addr;         // Physical address of tables_physical
} page_directory_t;

// Memory operations
void* memset(void* dest, int val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
void* memmove(void* dest, const void* src, size_t len);
int memcmp(const void* s1, const void* s2, size_t len);

// Memory management initialization
void memory_init(void);
void paging_init(void);

// Memory information
size_t get_total_memory(void);
size_t get_free_memory(void);
size_t get_used_memory(void);

// Page directory management
page_directory_t* create_page_directory(void);
page_directory_t* copy_page_directory(page_directory_t* src);
void free_page_directory(page_directory_t* dir);
page_directory_t* get_kernel_page_directory(void);
void switch_page_directory(page_directory_t* dir);

// Page and region management
page_t* alloc_page(void);
void free_page(page_t* page);
bool allocate_region(page_directory_t* dir, uint32_t start, uint32_t size, uint32_t flags);
void free_region(page_directory_t* dir, uint32_t start, uint32_t size);

// Memory mapping functions
void* mmap(void* addr, uint32_t length, int prot, int flags, int fd, uint32_t offset);
int munmap(void* addr, size_t length);

#endif // MEMORY_H
