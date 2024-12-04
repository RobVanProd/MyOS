# Memory Management API

## Overview

The Memory Management API provides a comprehensive interface for managing virtual memory, physical memory allocation, heap management, and memory mapping in MyOS.

## Virtual Memory Management

### Page Directory Operations

```c
void init_paging(void);
void switch_page_directory(page_directory_t *dir);
page_t *get_page(uint32_t address, int make, page_directory_t *dir);
```

#### init_paging()
Initializes the paging system and sets up the initial kernel page directory.

#### switch_page_directory()
Switches the current page directory by loading it into CR3.

Parameters:
- `dir`: Pointer to the new page directory structure

#### get_page()
Gets a page from the specified page directory, optionally creating it if it doesn't exist.

Parameters:
- `address`: Virtual address to get the page for
- `make`: Whether to create the page if it doesn't exist
- `dir`: Page directory to look in

Returns: Pointer to the page structure

### Page Frame Management

```c
void alloc_frame(page_t *page, int is_kernel, int is_writeable);
void free_frame(page_t *page);
```

#### alloc_frame()
Allocates a physical frame for a page.

Parameters:
- `page`: Page to allocate frame for
- `is_kernel`: Whether this is a kernel page
- `is_writeable`: Whether the page is writeable

#### free_frame()
Frees a physical frame.

Parameters:
- `page`: Page whose frame to free

### Memory Mapping

```c
void map_page(uint32_t virtual_addr, uint32_t physical_addr, int is_kernel, int is_writeable);
void unmap_page(uint32_t virtual_addr);
uint32_t get_physical_address(uint32_t virtual_addr);
```

#### map_page()
Maps a virtual address to a physical address.

Parameters:
- `virtual_addr`: Virtual address to map
- `physical_addr`: Physical address to map to
- `is_kernel`: Whether this is a kernel mapping
- `is_writeable`: Whether the mapping is writeable

#### unmap_page()
Unmaps a virtual address.

Parameters:
- `virtual_addr`: Virtual address to unmap

#### get_physical_address()
Gets the physical address for a virtual address.

Parameters:
- `virtual_addr`: Virtual address to translate

Returns: Physical address

## Heap Management

### Heap Operations

```c
void init_kheap(void);
heap_t *create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly);
```

#### init_kheap()
Initializes the kernel heap.

#### create_heap()
Creates a new heap.

Parameters:
- `start`: Start address
- `end`: End address
- `max`: Maximum size
- `supervisor`: Supervisor only?
- `readonly`: Read only?

Returns: Pointer to new heap structure

### Memory Allocation

```c
void *kmalloc(uint32_t size);
void *kmalloc_aligned(uint32_t size);
void *kmalloc_physical(uint32_t size, uint32_t *physical);
void *kmalloc_aligned_physical(uint32_t size, uint32_t *physical);
void kfree(void *ptr);
```

#### kmalloc()
Allocates kernel memory.

Parameters:
- `size`: Size in bytes to allocate

Returns: Pointer to allocated memory

#### kmalloc_aligned()
Allocates page-aligned kernel memory.

Parameters:
- `size`: Size in bytes to allocate

Returns: Pointer to allocated memory

#### kmalloc_physical()
Allocates kernel memory and returns physical address.

Parameters:
- `size`: Size in bytes to allocate
- `physical`: Pointer to store physical address

Returns: Pointer to allocated memory

#### kfree()
Frees allocated kernel memory.

Parameters:
- `ptr`: Pointer to memory to free

### Heap Information

```c
uint32_t get_free_memory(void);
uint32_t get_used_memory(void);
void get_heap_stats(uint32_t *total_blocks, uint32_t *free_blocks, uint32_t *largest_free);
```

#### get_free_memory()
Gets amount of free memory.

Returns: Free memory in bytes

#### get_used_memory()
Gets amount of used memory.

Returns: Used memory in bytes

#### get_heap_stats()
Gets detailed heap statistics.

Parameters:
- `total_blocks`: Pointer to store total blocks count
- `free_blocks`: Pointer to store free blocks count
- `largest_free`: Pointer to store largest free block size

## Memory Mapping

### mmap Operations

```c
void *mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset);
int munmap(void *addr, uint32_t length);
```

#### mmap()
Maps a region of memory.

Parameters:
- `addr`: Suggested address (or NULL)
- `length`: Length to map
- `prot`: Protection flags
- `flags`: Mapping flags
- `fd`: File descriptor (if file mapping)
- `offset`: Offset in file

Returns: Mapped address or NULL on failure

#### munmap()
Unmaps a region of memory.

Parameters:
- `addr`: Address to unmap
- `length`: Length to unmap

Returns: 0 on success, -1 on failure

### Protection Flags

```c
#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4
```

### Mapping Flags

```c
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
```

## Error Handling

### Page Fault Handler

```c
void page_fault_handler(void);
```

Handles page faults by:
1. Getting fault address from CR2
2. Checking error code for fault type
3. Allocating pages if needed
4. Handling copy-on-write
5. Handling demand paging

### Error Codes

```c
#define ERR_PRESENT   0x1
#define ERR_WRITE     0x2
#define ERR_USER      0x4
#define ERR_RESERVED  0x8
#define ERR_FETCH     0x10
```

## Examples

### Basic Page Allocation
```c
// Get a page
page_t *page = get_page(0x1000, 1, current_directory);

// Allocate a frame for it
alloc_frame(page, 0, 1);  // User mode, writeable

// Map it
map_page(0x1000, get_physical_address(0x1000), 0, 1);
```

### Heap Memory Usage
```c
// Allocate memory
void *ptr = kmalloc(1024);  // 1KB

// Use memory
memset(ptr, 0, 1024);

// Free memory
kfree(ptr);
```

### Memory Mapping
```c
// Map anonymous memory
void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

// Use memory
memset(addr, 0, 4096);

// Unmap
munmap(addr, 4096);
```

## Best Practices

1. Always check return values from allocation functions
2. Free all allocated memory to prevent leaks
3. Use aligned allocations for DMA or hardware buffers
4. Handle page faults appropriately
5. Use appropriate protection flags for security
6. Clean up mappings when no longer needed
7. Use COW when appropriate to save memory

## See Also

- [Process Management API](process.md)
- [Virtual Memory Documentation](../../core/memory/virtual.md)
- [Heap Management Documentation](../../core/memory/heap.md)
- [Memory Mapping Documentation](../../core/memory/mmap.md) 