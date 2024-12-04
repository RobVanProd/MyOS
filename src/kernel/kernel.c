#include "graphics.h"
#include "keyboard.h"
#include "memory.h"
#include "process.h"
#include "fs.h"
#include "mouse.h"
#include "network.h"
#include "sound.h"
#include "../apps/notepad.h"
#include "../apps/calculator.h"

// Global variables for GUI state
static int mouse_x = 160;
static int mouse_y = 100;
static bool mouse_left_button = false;

// Mouse event callback
void handle_mouse_event(mouse_event_t* event) {
    mouse_x = event->x;
    mouse_y = event->y;
    mouse_left_button = event->buttons & MOUSE_LEFT_BUTTON;
    
    if (mouse_left_button) {
        handle_window_click(mouse_x, mouse_y);
    }
}

// Sound callback
void handle_sound_callback(void* user_data) {
    // Handle sound buffer completion
}

void kernel_main(void) {
    // Initialize subsystems
    memory_init();
    process_init();
    fs_init();
    graphics_init();
    keyboard_init();
    mouse_init();
    network_init();
    sound_init();
    
    // Set up mouse callback
    mouse_set_callback(handle_mouse_event);
    
    // Create initial windows
    notepad_t* notepad = create_notepad(50, 50);
    calculator_t* calculator = create_calculator(300, 50);
    
    // Create a test file
    int fd = fs_open("/test.txt", FS_OPEN_CREATE | FS_OPEN_WRITE);
    if (fd >= 0) {
        const char* test_data = "Hello, File System!";
        fs_write(fd, test_data, strlen(test_data));
        fs_close(fd);
    }
    
    // Create a test process
    process_t* test_process = process_create("test", (void*)0x100000, PRIORITY_NORMAL, PROCESS_FLAG_USER);
    
    // Create a test sound buffer
    int sound_buf = sound_buffer_create(SOUND_FORMAT_PCM16, SOUND_CHANNEL_STEREO,
                                      SOUND_RATE_44100, 4096);
    if (sound_buf >= 0) {
        sound_buffer_set_callback(sound_buf, handle_sound_callback, NULL);
    }
    
    // Set up network interface
    network_interface_t test_interface = {
        .ip = ip_to_uint32("192.168.1.100"),
        .netmask = ip_to_uint32("255.255.255.0"),
        .gateway = ip_to_uint32("192.168.1.1")
    };
    network_interface_up(&test_interface);
    
    // Main event loop
    while (1) {
        // Clear screen with background color
        clear_screen(COLOR_BACKGROUND);
        
        // Draw all windows (from back to front)
        window_t* window = window_list;
        while (window) {
            draw_window(window);
            window = window->next;
        }
        
        // Draw mouse cursor
        draw_cursor(mouse_x, mouse_y);
        
        // Swap buffers to display frame
        swap_buffers();
        
        // Handle keyboard input
        char key = keyboard_get_key();
        if (key) {
            handle_window_key(key);
        }
        
        // Update sound system
        sound_update();
        
        // Run scheduler
        scheduler_tick();
    }
} 