#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"

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
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("\nType something to test the keyboard:\n> ");
    
    // Enable interrupts
    asm volatile("sti");
    
    // Main loop
    while (1) {
        // CPU can halt until next interrupt
        asm volatile("hlt");
    }
} 