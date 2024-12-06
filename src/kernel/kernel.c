#include <stddef.h>
#include <stdint.h>
#include "terminal.h"
#include "keyboard.h"
#include "memory.h"
#include "process.h"
#include "fs.h"
#include "mouse.h"
#include "sound.h"
#include "hal.h"
#include "driver.h"
#include "pci.h"
#include "isr.h"
#include "storage/ata.h"
#include "network/rtl8139.h"
#include "test_process.h"
#include "sound_buffer.h"

// Function declarations
void init_kernel(void);
void init_drivers(void);
void handle_sound_callback(void* buffer, uint32_t size);
void irq12_handler(registers_t* regs);
void test_process_entry(void);  // Test process entry point

// Global variables
static int sound_playing = 0;

// Sound callback
void handle_sound_callback(void* buffer, uint32_t size) {
    // Handle sound buffer filling here
    (void)buffer;   // Prevent unused parameter warning
    (void)size;     // Prevent unused parameter warning
}

// Mouse interrupt handler
void irq12_handler(registers_t* regs) {
    mouse_handle_interrupt(regs);
}

// Test process entry point
void test_process_entry(void) {
    while(1) {
        terminal_writestring("Test process running...\n");
        process_sleep(1000);  // Sleep for 1 second
    }
}

// Kernel entry point
void kernel_main(void) {
    // Initialize kernel subsystems
    init_kernel();
    
    // Initialize drivers
    init_drivers();
    
    // Initialize file system
    fs_init();
    
    // Initialize sound system
    sound_init();
    
    // Initialize process management
    process_init();
    
    // Set up sound callback
    sound_buffer_set_callback(0, handle_sound_callback);
    
    // Initialize mouse
    mouse_init();
    
    // Register mouse interrupt handler
    register_interrupt_handler(44, irq12_handler); // IRQ12 is mapped to interrupt 44
    
    // Clear terminal
    terminal_writestring("Welcome to MyOS!\n");
    terminal_writestring("Initializing system...\n");
    
    // Create test process
    process_create("test", test_process_entry);
    
    // Main kernel loop
    while (1) {
        // Process keyboard input
        if (keyboard_status()) {
            char c = keyboard_getchar();
            if (c) {
                terminal_putchar(c);
            }
        }
        
        // Update sound system
        sound_update();
        
        // Halt CPU until next interrupt
        __asm__ volatile("hlt");
    }
}

// Initialize kernel subsystems
void init_kernel(void) {
    // Initialize terminal
    terminal_initialize();
    
    // Initialize memory management
    memory_init();
    
    // Initialize process management
    process_init();
    
    // Initialize keyboard
    keyboard_init();
}

// Initialize drivers
void init_drivers(void) {
    // Initialize HAL
    hal_interrupt_init();
    
    // Initialize driver subsystem
    driver_init_all();
    
    // Initialize PCI
    pci_init();
    
    // Initialize storage drivers
    driver_t* storage_driver = driver_find_by_type(DRIVER_TYPE_STORAGE);
    if (storage_driver) {
        driver_register(storage_driver);
    }
    
    // Initialize network drivers
    driver_t* network_driver = driver_find_by_type(DRIVER_TYPE_NETWORK);
    if (network_driver) {
        driver_register(network_driver);
    }
}