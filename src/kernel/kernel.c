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

// Function declarations
void init_kernel(void);
void init_drivers(void);
void handle_sound_callback(void* user_data);

// Global variables
static int sound_playing = 0;

// Sound callback
void handle_sound_callback(void* user_data) {
    (void)user_data; // Suppress unused parameter warning
    sound_playing = 0;
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
    
    // Set up sound callback
    sound_buffer_set_callback(0, handle_sound_callback, NULL);
    
    // Initialize mouse
    mouse_init();
    
    // Clear terminal
    terminal_initialize();
    terminal_writestring("Welcome to MyOS!\n");
    terminal_writestring("Initializing system...\n");
    
    // Create test process
    process_create("test", (void*)0x100000, PRIORITY_NORMAL, PROCESS_FLAG_USER);
    
    // Main kernel loop
    while (1) {
        // Process keyboard input
        char c = keyboard_getchar();
        if (c) {
            terminal_putchar(c);
        }
        
        // Process mouse events
        void irq12_handler(registers_t* regs) {
            mouse_handle_interrupt(regs);
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
    
    // Register storage drivers
    driver_t* storage_driver = driver_find_by_type(DRIVER_TYPE_STORAGE);
    if (storage_driver) {
        driver_register(storage_driver);
    }
    
    // Register network drivers
    driver_t* network_driver = driver_find_by_type(DRIVER_TYPE_NETWORK);
    if (network_driver) {
        driver_register(network_driver);
    }
}