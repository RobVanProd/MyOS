#include "mmap.h"
#include "paging.h"
#include "memory.h"
#include "terminal.h"
#include <string.h>

// Declare kernel_directory as external
extern page_directory_t *kernel_directory;

// Frame allocation function (should be in paging.c)
static uint32_t find_free_frame(void) {
    static uint32_t frame = 0;
    // Simple frame allocation strategy - just increment
    // In a real OS, you'd want to track free frames
    return frame++;
}

// Memory mapping entry
typedef struct mmap_entry {
    uint32_t start_addr;
    uint32_t length;
    int prot;
    int flags;
    int fd;
    uint32_t offset;
    struct mmap_entry* next;
} mmap_entry_t;

// List of memory mappings
static mmap_entry_t* mmap_list = NULL;

// Start address for memory mappings
static uint32_t mmap_start_addr = 0xD0000000; // 3.25GB

// Maximum number of memory mappings
#define MAX_MAPPINGS 1024

// Current number of mappings
static int num_mappings = 0;

// Initialize memory mapping
void init_mmap(void) {
    mmap_list = NULL;
    num_mappings = 0;
}

// Find a mapping that contains the given address
static mmap_entry_t* get_mapping(uint32_t addr) {
    mmap_entry_t* entry = mmap_list;
    while (entry) {
        if (addr >= entry->start_addr && addr < entry->start_addr + entry->length) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

// Add a new mapping entry
static mmap_entry_t* add_mapping(uint32_t start, uint32_t length, int prot, 
                                int flags, int fd, uint32_t offset) {
    if (num_mappings >= MAX_MAPPINGS) {
        kprintf("Maximum number of mappings reached\n");
        return NULL;
    }
    
    mmap_entry_t* entry = kmalloc(sizeof(mmap_entry_t));
    if (!entry) {
        kprintf("Failed to allocate memory for mapping entry\n");
        return NULL;
    }
    
    entry->start_addr = start;
    entry->length = length;
    entry->prot = prot;
    entry->flags = flags;
    entry->fd = fd;
    entry->offset = offset;
    
    // Add to list
    entry->next = mmap_list;
    mmap_list = entry;
    num_mappings++;
    
    return entry;
}

// Remove a mapping entry
static void remove_mapping(mmap_entry_t* entry) {
    if (!entry) return;
    
    if (mmap_list == entry) {
        mmap_list = entry->next;
    } else {
        mmap_entry_t* curr = mmap_list;
        while (curr && curr->next != entry) {
            curr = curr->next;
        }
        if (curr) {
            curr->next = entry->next;
        }
    }
    
    num_mappings--;
    kfree(entry);
}

// Find a suitable address for mapping
static void* find_mmap_space(uint32_t length, uint32_t hint) {
    if (hint) {
        // Check if hint address is available
        uint32_t end = hint + length;
        mmap_entry_t* entry = mmap_list;
        while (entry) {
            if (!(end <= entry->start_addr || hint >= entry->start_addr + entry->length)) {
                hint = 0; // Conflict found, ignore hint
                break;
            }
            entry = entry->next;
        }
        if (hint) return (void*)hint;
    }
    
    // Find a free region
    uint32_t addr = mmap_start_addr;
    uint32_t len_pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    
    mmap_entry_t* entry = mmap_list;
    while (entry) {
        if (addr + (len_pages * PAGE_SIZE) <= entry->start_addr) {
            // Found a gap
            break;
        }
        addr = entry->start_addr + entry->length;
        entry = entry->next;
    }
    
    // Check if address is too high
    if (addr + length > 0xFFFFFFFF) {
        kprintf("No suitable address space found for mapping\n");
        return NULL;
    }
    
    return (void*)addr;
}

// Handle page fault in mapped region
int handle_mmap_fault(uint32_t fault_addr) {
    mmap_entry_t* entry = get_mapping(fault_addr);
    if (!entry) return 0; // Not a mapped region
    
    // Calculate page-aligned address
    uint32_t page_addr = fault_addr & ~0xFFF;
    
    // Check protection flags
    uint32_t page_flags = PAGE_PRESENT;
    if (entry->prot & PROT_WRITE) page_flags |= PAGE_WRITE;
    if (!(entry->flags & MAP_PRIVATE)) page_flags |= PAGE_USER;
    
    // Allocate physical frame
    uint32_t frame = find_free_frame();
    if (!frame) {
        kprintf("No free frames available for mapping\n");
        return -1;
    }
    
    // Map the frame
    map_page(page_addr, frame, page_flags);
    
    // Initialize page content
    if (entry->fd >= 0 && !(entry->flags & MAP_ANONYMOUS)) {
        // File-backed mapping
        uint32_t offset = fault_addr - entry->start_addr + entry->offset;
        if (fs_seek(entry->fd, offset) < 0) {
            kprintf("Failed to seek in file for mapping\n");
            return -1;
        }
        
        uint8_t* page = (uint8_t*)page_addr;
        if (fs_read(entry->fd, page, PAGE_SIZE) < 0) {
            kprintf("Failed to read file for mapping\n");
            return -1;
        }
    } else {
        // Anonymous mapping - zero the page
        memset((void*)page_addr, 0, PAGE_SIZE);
    }
    
    return 1;
}

// Create a new memory mapping
void* do_mmap(void* addr, uint32_t length, int prot, int flags, int fd, uint32_t offset) {
    // Validate parameters
    if (length == 0) {
        kprintf("Invalid mapping length\n");
        return MAP_FAILED;
    }
    
    // Align length to page boundary
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Find address if not fixed
    if (!(flags & MAP_FIXED)) {
        addr = find_mmap_space(length, (uint32_t)addr);
        if (!addr) {
            return MAP_FAILED;
        }
    } else if ((uint32_t)addr & (PAGE_SIZE - 1)) {
        // Fixed mapping must be page-aligned
        kprintf("Fixed mapping address not page-aligned\n");
        return MAP_FAILED;
    }
    
    // Check if address range is free
    uint32_t start = (uint32_t)addr;
    mmap_entry_t* existing = get_mapping(start);
    if (existing) {
        kprintf("Address range already mapped\n");
        return MAP_FAILED;
    }
    
    // Add mapping entry
    mmap_entry_t* entry = add_mapping(start, length, prot, flags, fd, offset);
    if (!entry) {
        return MAP_FAILED;
    }
    
    // Reserve virtual address space
    for (uint32_t i = 0; i < length; i += PAGE_SIZE) {
        page_t* page = get_page(start + i, 1, current_directory);
        if (!page) {
            // Failed to allocate page table entry
            remove_mapping(entry);
            return MAP_FAILED;
        }
        page->present = 0; // Will be handled by page fault
        page->rw = (prot & PROT_WRITE) ? 1 : 0;
        page->user = 1;
    }
    
    return addr;
}

// Unmap a memory region
int do_munmap(void* addr, uint32_t length) {
    uint32_t start = (uint32_t)addr;
    
    // Validate parameters
    if ((start & (PAGE_SIZE - 1)) || length == 0) {
        kprintf("Invalid munmap parameters\n");
        return -1;
    }
    
    // Align length to page boundary
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Find mapping
    mmap_entry_t* entry = get_mapping(start);
    if (!entry || entry->start_addr != start) {
        kprintf("Invalid munmap address\n");
        return -1;
    }
    
    // Unmap pages
    for (uint32_t i = 0; i < length; i += PAGE_SIZE) {
        unmap_page(start + i);
    }
    
    // Remove mapping entry
    remove_mapping(entry);
    
    return 0;
}

// Dump memory mappings
void dump_mappings(void) {
    terminal_writestring("\nMemory Mappings:\n");
    terminal_writestring("-----------------\n");
    
    mmap_entry_t* entry = mmap_list;
    while (entry) {
        // Convert addresses to hex strings
        char start[9], end[9];
        uint32_t start_addr = entry->start_addr;
        uint32_t end_addr = entry->start_addr + entry->length;
        
        for (int i = 0; i < 8; i++) {
            int digit = (start_addr >> ((7-i) * 4)) & 0xF;
            start[i] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
            
            digit = (end_addr >> ((7-i) * 4)) & 0xF;
            end[i] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
        }
        start[8] = end[8] = '\0';
        
        terminal_writestring("0x");
        terminal_writestring(start);
        terminal_writestring(" - 0x");
        terminal_writestring(end);
        terminal_writestring(" : ");
        
        // Print protection flags
        if (entry->prot & PROT_READ) terminal_writestring("R");
        if (entry->prot & PROT_WRITE) terminal_writestring("W");
        if (entry->prot & PROT_EXEC) terminal_writestring("X");
        
        terminal_writestring("\n");
        entry = entry->next;
    }
}