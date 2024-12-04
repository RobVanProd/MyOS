#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "paging.h"
#include "kheap.h"

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
    
    // Print memory information
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    print_memory_info();
    
    terminal_writestring("\nType something to test the keyboard:\n> ");
    
    // Enable interrupts
    asm volatile("sti");
    
    // Main loop
    while (1) {
        // CPU can halt until next interrupt
        asm volatile("hlt");
    }
} 