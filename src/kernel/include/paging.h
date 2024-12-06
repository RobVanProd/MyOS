#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "memory.h"

// Page directory entry flags
#define PAGE_PRESENT    0x1
#define PAGE_WRITE     0x2
#define PAGE_USER      0x4
#define PAGE_ACCESSED  0x20
#define PAGE_DIRTY     0x40

// Functions for page directory management
void init_paging(void);
void switch_page_directory(page_directory_t* dir);
page_directory_t* copy_page_directory(page_directory_t* src);
void map_page(void* physaddr, void* virtualaddr, uint32_t flags);
void unmap_page(void* virtualaddr);
void* get_physical_address(void* virtualaddr);

#endif /* PAGING_H */
