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

// List of memory mappings
static mmap_entry_t *mmap_list = NULL;

// Start address for memory mappings
static uint32_t mmap_start_addr = 0xD0000000; // 3.25GB

// Initialize memory mapping
void init_mmap(void) {
    mmap_list = NULL;
}

// Find a suitable address for mapping
static void *find_mmap_space(uint32_t length, int fixed_addr) {
    if (fixed_addr) {
        return (void*)fixed_addr;
    }
    
    uint32_t addr = mmap_start_addr;
    uint32_t len_pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Find a free region
    mmap_entry_t *entry = mmap_list;
    while (entry) {
        if (addr + (len_pages * PAGE_SIZE) <= entry->start_addr) {
            // Found a gap
            break;
        }
        addr = entry->start_addr + entry->length;
        entry = entry->next;
    }
    
    return (void*)addr;
}

// Add a new mapping entry
static mmap_entry_t *add_mapping(uint32_t start, uint32_t length, int prot, int flags, int fd, uint32_t offset) {
    mmap_entry_t *entry = kmalloc(sizeof(mmap_entry_t));
    if (!entry) return NULL;
    
    entry->start_addr = start;
    entry->length = length;
    entry->flags = prot;
    entry->file_offset = offset;
    entry->fd = fd;
    
    // Insert into list (sorted by address)
    if (!mmap_list || mmap_list->start_addr > start) {
        entry->next = mmap_list;
        mmap_list = entry;
    } else {
        mmap_entry_t *curr = mmap_list;
        while (curr->next && curr->next->start_addr < start) {
            curr = curr->next;
        }
        entry->next = curr->next;
        curr->next = entry;
    }
    
    return entry;
}

// Remove a mapping entry
static void remove_mapping(mmap_entry_t *entry) {
    if (!entry) return;
    
    if (mmap_list == entry) {
        mmap_list = entry->next;
    } else {
        mmap_entry_t *curr = mmap_list;
        while (curr && curr->next != entry) {
            curr = curr->next;
        }
        if (curr) {
            curr->next = entry->next;
        }
    }
    
    kfree(entry);
}

void *do_mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset) {
    // Align length to page boundary
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Find address if not fixed
    if (!(flags & MAP_FIXED)) {
        addr = find_mmap_space(length, (uint32_t)addr);
    }
    
    if (!addr) return NULL;
    
    // Check if address range is free
    uint32_t start = (uint32_t)addr;
    mmap_entry_t *existing = get_mapping(start);
    if (existing) {
        terminal_writestring("Address range already mapped\n");
        return NULL;
    }
    
    // Add mapping entry
    mmap_entry_t *entry = add_mapping(start, length, prot, flags, fd, offset);
    if (!entry) {
        terminal_writestring("Failed to create mapping entry\n");
        return NULL;
    }
    
    // Map pages as demand-paged (not present)
    for (uint32_t i = 0; i < length; i += PAGE_SIZE) {
        page_t *page = get_page(start + i, 1, kernel_directory);
        page->present = 0;
        page->rw = (prot & PROT_WRITE) ? 1 : 0;
        page->user = 1;
    }
    
    return addr;
}

int do_munmap(void *addr, uint32_t length) {
    uint32_t start = (uint32_t)addr;
    mmap_entry_t *entry = get_mapping(start);
    
    if (!entry || entry->start_addr != start) {
        terminal_writestring("Invalid munmap address\n");
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

int handle_mmap_fault(uint32_t fault_addr) {
    mmap_entry_t *entry = get_mapping(fault_addr);
    if (!entry) return 0; // Not a mapped region
    
    // Calculate offset into mapping
    uint32_t offset = fault_addr - entry->start_addr;
    
    // Allocate a frame
    page_t *page = get_page(fault_addr, 0, kernel_directory);
    if (!page) return -1;
    
    uint32_t frame = find_free_frame();
    if (!frame) {
        terminal_writestring("No free frames for page fault\n");
        return -1;
    }
    
    // Map the frame
    page->present = 1;
    page->frame = frame / PAGE_SIZE;
    page->rw = (entry->flags & PROT_WRITE) ? 1 : 0;
    page->user = 1;
    
    // If file-backed, load the page
    if (!(entry->flags & MAP_ANONYMOUS) && entry->fd >= 0) {
        // TODO: Implement file reading
        // For now, just zero the page
        uint8_t *page_addr = (uint8_t*)(fault_addr & ~0xFFF);
        for (uint32_t i = 0; i < PAGE_SIZE; i++) {
            page_addr[i] = 0;
        }
    } else {
        // Zero the page for anonymous mappings
        uint8_t *page_addr = (uint8_t*)(fault_addr & ~0xFFF);
        for (uint32_t i = 0; i < PAGE_SIZE; i++) {
            page_addr[i] = 0;
        }
    }
    
    return 1;
}

mmap_entry_t *get_mapping(uint32_t addr) {
    mmap_entry_t *entry = mmap_list;
    while (entry) {
        if (addr >= entry->start_addr && addr < entry->start_addr + entry->length) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

void dump_mappings(void) {
    terminal_writestring("\nMemory Mappings:\n");
    terminal_writestring("-----------------\n");
    
    mmap_entry_t *entry = mmap_list;
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
        if (entry->flags & PROT_READ) terminal_writestring("R");
        if (entry->flags & PROT_WRITE) terminal_writestring("W");
        if (entry->flags & PROT_EXEC) terminal_writestring("X");
        
        terminal_writestring("\n");
        entry = entry->next;
    }
}