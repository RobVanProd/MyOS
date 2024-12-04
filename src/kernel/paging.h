#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

// Page size is 4KB
#define PAGE_SIZE 0x1000

// Page table/directory entry flags
#define PAGE_PRESENT   0x1
#define PAGE_WRITE     0x2
#define PAGE_USER      0x4
#define PAGE_ACCESSED  0x20
#define PAGE_DIRTY     0x40

// Page directory and table structures
typedef struct page {
    uint32_t present    : 1;   // Page present in memory
    uint32_t rw         : 1;   // Read-only if clear, readwrite if set
    uint32_t user       : 1;   // Supervisor level only if clear
    uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;   // Has the page been written to since last refresh?
    uint32_t unused     : 7;   // Unused and reserved bits
    uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table {
    page_t pages[1024];
} page_table_t;

typedef struct page_directory {
    // Array of pointers to page tables
    page_table_t *tables[1024];
    // Array of physical addresses of page tables
    uint32_t tables_physical[1024];
    // Physical address of tables_physical
    uint32_t physical_addr;
} page_directory_t;

// Initialize paging
void init_paging(void);

// Load page directory into CR3 register
void switch_page_directory(page_directory_t *dir);

// Get a page from the current page directory
page_t *get_page(uint32_t address, int make, page_directory_t *dir);

// Page fault handler
void page_fault_handler(void);

// Allocate a frame
void alloc_frame(page_t *page, int is_kernel, int is_writeable);

// Free a frame
void free_frame(page_t *page);

// Map virtual address to physical address
void map_page(uint32_t virtual_addr, uint32_t physical_addr, int is_kernel, int is_writeable);

// Unmap virtual address
void unmap_page(uint32_t virtual_addr);

// Get the physical address for a virtual address
uint32_t get_physical_address(uint32_t virtual_addr);

#endif 