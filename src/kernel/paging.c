#include "paging.h"
#include "string.h"
#include "kprintf.h"
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

void page_fault_handler(registers_t* regs) {
    // Get the faulting address from CR2
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    // The error code gives us details of what happened
    int present = regs->err_code & 0x1;    // Page present
    int rw = regs->err_code & 0x2;         // Write operation?
    int us = regs->err_code & 0x4;         // User mode?
    int reserved = regs->err_code & 0x8;   // Reserved bits overwritten?
    int id = regs->err_code & 0x10;        // Instruction fetch?
    
    // First try to handle it as a memory mapping fault
    if (!present && handle_mmap_fault(faulting_address)) {
        return; // Successfully handled by memory mapping
    }
    
    // Check if it's a kernel page fault
    if (!us && !current_process->flags & PROCESS_FLAG_KERNEL) {
        kprintf("Page fault in kernel mode! System halted.\n");
        for(;;);
    }
    
    // Check if it's a stack growth request
    if (!present && !reserved && current_process) {
        uint32_t stack_base = current_process->stack_base;
        uint32_t stack_limit = stack_base - MAX_STACK_SIZE;
        
        if (faulting_address >= stack_limit && faulting_address < stack_base) {
            // Align address to page boundary
            uint32_t page_addr = faulting_address & ~0xFFF;
            
            // Allocate new stack page
            if (allocate_region(current_directory, page_addr, PAGE_SIZE, 
                              PAGE_PRESENT | PAGE_WRITE | PAGE_USER)) {
                return; // Stack growth successful
            }
        }
    }
    
    // If we get here, it's a real page fault
    kprintf("Page fault at 0x%x ( ", faulting_address);
    if (present) kprintf("present ");
    if (rw) kprintf("read-only ");
    if (us) kprintf("user-mode ");
    if (reserved) kprintf("reserved ");
    if (id) kprintf("instruction-fetch ");
    kprintf(")\n");
    
    // Print call stack if available
    if (current_process) {
        kprintf("Process: %s (PID: %d)\n", 
                current_process->name, current_process->pid);
        kprintf("EIP: 0x%x\n", regs->eip);
        
        // Print stack trace
        uint32_t* ebp = (uint32_t*)regs->ebp;
        kprintf("Stack trace:\n");
        for (int i = 0; i < 5 && ebp; i++) {
            uint32_t eip = ebp[1];
            kprintf("  [%d] 0x%x\n", i, eip);
            ebp = (uint32_t*)ebp[0];
        }
    }
    
    // Halt the system or kill the process
    if (!current_process || current_process->flags & PROCESS_FLAG_KERNEL) {
        kprintf("Kernel panic: unhandled page fault\n");
        for(;;);
    } else {
        kprintf("Killing process %s due to page fault\n", current_process->name);
        process_exit(current_process, -1);
    }
}

// Function to map a virtual page to a physical frame
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    page_t* page = get_page(virtual_addr, 1, current_directory);
    if (!page) {
        kprintf("Failed to get page for virtual address %x\n", virtual_addr);
        return;
    }
    
    page->present = 1;
    page->rw = (flags & PAGE_WRITE) ? 1 : 0;
    page->user = (flags & PAGE_USER) ? 1 : 0;
    page->frame = physical_addr >> 12;
}

// Function to unmap a virtual page
void unmap_page(uint32_t virtual_addr) {
    page_t* page = get_page(virtual_addr, 0, current_directory);
    if (!page) {
        return;
    }
    
    page->present = 0;
    page->frame = 0;
}

// Function to create a new page directory
page_directory_t* create_page_directory(void) {
    uint32_t phys;
    page_directory_t* dir = (page_directory_t*)kmalloc_aligned_physical(sizeof(page_directory_t), &phys);
    memset(dir, 0, sizeof(page_directory_t));
    
    // Copy kernel page tables
    for (int i = 0; i < 1024; i++) {
        if (kernel_directory->tables[i]) {
            dir->tables[i] = kernel_directory->tables[i];
            dir->tables_physical[i] = kernel_directory->tables_physical[i];
        }
    }
    
    return dir;
}

// Function to free a page directory
void free_page_directory(page_directory_t* dir) {
    if (!dir) return;
    
    // Free all page tables
    for (int i = 0; i < 1024; i++) {
        if (dir->tables[i] && dir->tables[i] != kernel_directory->tables[i]) {
            // Free all pages in this table
            for (int j = 0; j < 1024; j++) {
                if (dir->tables[i]->pages[j].present) {
                    free_frame(&dir->tables[i]->pages[j]);
                }
            }
            kfree(dir->tables[i]);
        }
    }
    
    kfree(dir);
}

// Function to get the kernel's page directory
page_directory_t* get_kernel_page_directory(void) {
    return kernel_directory;
}