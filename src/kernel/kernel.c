#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "paging.h"
#include "kheap.h"
#include "mmap.h"

void print_memory_info(void) {
    uint32_t free_mem = get_free_memory();
    uint32_t used_mem = get_used_memory();
    
    terminal_writestring("\nMemory Information:\n");
    terminal_writestring("----------------\n");
    
    // Print free memory
    terminal_writestring("Free memory: ");
    char free_mem_str[32];
    int idx = 0;
    uint32_t temp = free_mem / 1024; // Convert to KB
    do {
        free_mem_str[idx++] = '0' + (temp % 10);
        temp /= 10;
    } while (temp > 0);
    free_mem_str[idx] = '\0';
    // Print in reverse
    while (--idx >= 0) {
        terminal_putchar(free_mem_str[idx]);
    }
    terminal_writestring(" KB\n");
    
    // Print used memory
    terminal_writestring("Used memory: ");
    char used_mem_str[32];
    idx = 0;
    temp = used_mem / 1024; // Convert to KB
    do {
        used_mem_str[idx++] = '0' + (temp % 10);
        temp /= 10;
    } while (temp > 0);
    used_mem_str[idx] = '\0';
    // Print in reverse
    while (--idx >= 0) {
        terminal_putchar(used_mem_str[idx]);
    }
    terminal_writestring(" KB\n");
    
    // Print heap statistics
    uint32_t total_blocks, free_blocks, largest_free;
    get_heap_stats(&total_blocks, &free_blocks, &largest_free);
    
    terminal_writestring("\nHeap Statistics:\n");
    terminal_writestring("---------------\n");
    terminal_writestring("Total blocks: ");
    char total_str[32];
    idx = 0;
    temp = total_blocks;
    do {
        total_str[idx++] = '0' + (temp % 10);
        temp /= 10;
    } while (temp > 0);
    while (--idx >= 0) {
        terminal_putchar(total_str[idx]);
    }
    terminal_writestring("\nFree blocks: ");
    idx = 0;
    temp = free_blocks;
    do {
        total_str[idx++] = '0' + (temp % 10);
        temp /= 10;
    } while (temp > 0);
    while (--idx >= 0) {
        terminal_putchar(total_str[idx]);
    }
    terminal_writestring("\nLargest free block: ");
    idx = 0;
    temp = largest_free;
    do {
        total_str[idx++] = '0' + (temp % 10);
        temp /= 10;
    } while (temp > 0);
    while (--idx >= 0) {
        terminal_putchar(total_str[idx]);
    }
    terminal_writestring(" bytes\n");
}

void test_memory_mapping(void) {
    terminal_writestring("\nTesting memory mapping...\n");
    
    // Test anonymous mapping
    void *anon_map = do_mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (anon_map) {
        terminal_writestring("Created anonymous mapping at 0x");
        char addr[9];
        uint32_t temp = (uint32_t)anon_map;
        for (int i = 0; i < 8; i++) {
            int digit = (temp >> ((7-i) * 4)) & 0xF;
            addr[i] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
        }
        addr[8] = '\0';
        terminal_writestring(addr);
        terminal_writestring("\n");
        
        // Test writing to the mapping
        uint8_t *test_ptr = (uint8_t*)anon_map;
        *test_ptr = 42; // This should cause a page fault and allocation
        
        terminal_writestring("Successfully wrote to mapped memory\n");
    } else {
        terminal_writestring("Failed to create anonymous mapping\n");
    }
    
    // Dump all mappings
    dump_mappings();
}

void kernel_main(void) {
    // Initialize terminal for output
    terminal_initialize();
    
    // Initialize GDT
    terminal_writestring("Initializing GDT...\n");
    gdt_init();
    terminal_writestring("GDT initialized successfully!\n");
    
    // Initialize IDT
    terminal_writestring("Initializing IDT...\n");
    idt_init();
    terminal_writestring("IDT initialized successfully!\n");
    
    // Initialize PIC
    terminal_writestring("Initializing PIC...\n");
    pic_init();
    terminal_writestring("PIC initialized successfully!\n");
    
    // Initialize heap
    terminal_writestring("Initializing kernel heap...\n");
    init_kheap();
    terminal_writestring("Kernel heap initialized successfully!\n");
    
    // Initialize paging
    terminal_writestring("Initializing paging...\n");
    init_paging();
    terminal_writestring("Paging initialized successfully!\n");
    
    // Initialize memory mapping
    terminal_writestring("Initializing memory mapping...\n");
    init_mmap();
    terminal_writestring("Memory mapping initialized successfully!\n");
    
    // Initialize keyboard
    terminal_writestring("Initializing keyboard...\n");
    keyboard_init();
    terminal_writestring("Keyboard initialized successfully!\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("\nWelcome to MyOS!\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("System initialization complete.\n");
    terminal_writestring("Features initialized:\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("- VGA text mode\n");
    terminal_writestring("- Global Descriptor Table (GDT)\n");
    terminal_writestring("- Interrupt Descriptor Table (IDT)\n");
    terminal_writestring("- Programmable Interrupt Controller (PIC)\n");
    terminal_writestring("- Keyboard driver\n");
    terminal_writestring("- Paging enabled\n");
    terminal_writestring("- Kernel heap initialized\n");
    terminal_writestring("- Memory mapping support\n");
    
    // Print memory information
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    print_memory_info();
    
    // Test memory mapping
    test_memory_mapping();
    
    terminal_writestring("\nType something to test the keyboard:\n> ");
    
    // Enable interrupts
    asm volatile("sti");
    
    // Main loop
    while (1) {
        // CPU can halt until next interrupt
        asm volatile("hlt");
    }
} 