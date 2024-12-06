#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <isr.h>  // For registers_t

// Page size is 4KB
#define PAGE_SIZE 0x1000

// Page flags
#define PAGE_PRESENT     0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4
#define PAGE_ACCESSED   0x20
#define PAGE_DIRTY      0x40

typedef struct page {
    uint32_t present    : 1;   // Page present in memory
    uint32_t rw        : 1;   // Read-only if clear, readwrite if set
    uint32_t user      : 1;   // Supervisor level only if clear
    uint32_t accessed  : 1;   // Has the page been accessed since last refresh?
    uint32_t dirty     : 1;   // Has the page been written to since last refresh?
    uint32_t unused    : 7;   // Amalgamation of unused and reserved bits
    uint32_t frame     : 20;  // Frame address (shifted right 12 bits)
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

// Function declarations
void init_paging(void);
void switch_page_directory(page_directory_t *dir);
page_t *get_page(uint32_t address, int make, page_directory_t *dir);
void page_fault(registers_t *regs);
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(uint32_t virtual_addr);
page_directory_t* create_page_directory(void);
void free_page_directory(page_directory_t* dir);
page_directory_t* get_kernel_page_directory(void);

#endif