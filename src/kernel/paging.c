#include "paging.h"
#include "kheap.h"
#include "terminal.h"

// The kernel's page directory
page_directory_t *kernel_directory = 0;

// The current page directory
page_directory_t *current_directory = 0;

// A bitset of frames - used or free
uint32_t *frames;
uint32_t nframes;

// Defined in kheap.c
extern uint32_t placement_address;

// Macros used in the bitset algorithms
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

// Static function to set a bit in the frames bitset
static void set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr/0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr/0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set
static uint32_t test_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr/0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame
static uint32_t first_frame(void) {
    uint32_t i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
        if (frames[i] != 0xFFFFFFFF) {
            // At least one bit is free here
            for (j = 0; j < 32; j++) {
                uint32_t toTest = 0x1 << j;
                if (!(frames[i] & toTest)) {
                    return i*32 + j;
                }
            }
        }
    }
    return (uint32_t)-1;
}

// Function to allocate a frame
void alloc_frame(page_t *page, int is_kernel, int is_writeable) {
    if (page->frame != 0) {
        return; // Frame was already allocated
    }
    uint32_t idx = first_frame();
    if (idx == (uint32_t)-1) {
        terminal_writestring("No free frames!\n");
        return;
    }
    set_frame(idx * 0x1000);
    page->present = 1;
    page->rw = (is_writeable) ? 1 : 0;
    page->user = (is_kernel) ? 0 : 1;
    page->frame = idx;
}

// Function to deallocate a frame
void free_frame(page_t *page) {
    uint32_t frame;
    if (!(frame = page->frame)) {
        return; // The page didn't have an allocated frame
    }
    clear_frame(frame * 0x1000);
    page->frame = 0x0;
}

void init_paging(void) {
    // Size of physical memory. For now, we assume 16MB
    uint32_t mem_end_page = 0x1000000;
    
    nframes = mem_end_page / 0x1000;
    frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes));
    
    // Set all frames to zero (unused)
    for (uint32_t i = 0; i < INDEX_FROM_BIT(nframes); i++) {
        frames[i] = 0;
    }
    
    // Create a page directory
    kernel_directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t));
    
    // Clear the page directory
    for (int i = 0; i < 1024; i++) {
        kernel_directory->tables[i] = 0;
        kernel_directory->tables_physical[i] = 0;
    }
    
    // Identity map the first 4MB
    for (uint32_t i = 0; i < 0x400000; i += 0x1000) {
        get_page(i, 1, kernel_directory);
    }
    
    // Now allocate those pages we mapped earlier
    for (uint32_t i = 0; i < placement_address + 0x1000; i += 0x1000) {
        alloc_frame(get_page(i, 1, kernel_directory), 1, 0);
    }
    
    // Register page fault handler
    // TODO: Register with IDT
    
    // Enable paging
    switch_page_directory(kernel_directory);
}

void switch_page_directory(page_directory_t *dir) {
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(dir->physical_addr));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(uint32_t address, int make, page_directory_t *dir) {
    // Turn the address into an index
    address /= 0x1000;
    // Find the page table containing this address
    uint32_t table_idx = address / 1024;
    
    if (dir->tables[table_idx]) { // If this table is already assigned
        return &dir->tables[table_idx]->pages[address % 1024];
    } else if(make) {
        uint32_t tmp;
        dir->tables[table_idx] = (page_table_t*)kmalloc_aligned_physical(sizeof(page_table_t), &tmp);
        dir->tables_physical[table_idx] = tmp | 0x7; // Present, RW, US
        return &dir->tables[table_idx]->pages[address % 1024];
    }
    return 0;
}

void page_fault_handler(void) {
    // A page fault has occurred
    // The faulting address is stored in the CR2 register
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    // The error code gives us details of what happened
    int present = 0;   // Page not present
    int rw = 0;        // Write operation?
    int us = 0;        // Processor was in user-mode?
    int reserved = 0;  // Overwritten CPU-reserved bits of page entry?
    
    terminal_writestring("Page fault! ( ");
    if (present) terminal_writestring("present ");
    if (rw) terminal_writestring("read-only ");
    if (us) terminal_writestring("user-mode ");
    if (reserved) terminal_writestring("reserved ");
    terminal_writestring(") at address 0x");
    
    // Print the address in hexadecimal
    char hex[9];
    for (int i = 7; i >= 0; i--) {
        int digit = (faulting_address >> (i * 4)) & 0xF;
        hex[7-i] = digit < 10 ? digit + '0' : digit - 10 + 'A';
    }
    hex[8] = '\0';
    terminal_writestring(hex);
    terminal_writestring("\n");
} 