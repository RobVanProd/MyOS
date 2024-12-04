#ifndef MMAP_H
#define MMAP_H

#include <stdint.h>

// Memory protection flags
#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

// Memory mapping flags
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20

// Memory mapping entry structure
typedef struct mmap_entry {
    uint32_t start_addr;      // Start of mapped region
    uint32_t length;          // Length of mapped region
    uint32_t flags;           // Protection flags
    uint32_t file_offset;     // Offset in file (if file-backed)
    int fd;                   // File descriptor (if file-backed)
    struct mmap_entry *next;  // Next entry in list
} mmap_entry_t;

// Initialize memory mapping subsystem
void init_mmap(void);

// Map memory region
void *do_mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset);

// Unmap memory region
int do_munmap(void *addr, uint32_t length);

// Handle page fault for mapped region
int handle_mmap_fault(uint32_t fault_addr);

// Get mapping information for address
mmap_entry_t *get_mapping(uint32_t addr);

// Debug function to dump all mappings
void dump_mappings(void);

#endif 