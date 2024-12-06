 #include <mouse.h>
#include <io.h>
#include <pic.h>
#include "graphics.h"
#include <isr.h>
#include <stddef.h>

// Mouse callback type
typedef void (*mouse_callback_t)(mouse_state_t*);

// Define mouse state
static mouse_state_t mouse_state = {0, 0, 0};
static mouse_callback_t mouse_callback = NULL;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
        return;
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
        return;
    }
}

void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, data);
}

uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_handle_interrupt(registers_t* regs) {
    (void)regs; // Unused parameter
    
    uint8_t status = inb(MOUSE_STATUS_PORT);
    if (!(status & 0x20)) {
        return; // No mouse data to read
    }

    uint8_t data = inb(MOUSE_DATA_PORT);
    
    switch(mouse_cycle) {
        case 0:
            mouse_byte[0] = data;
            if (data & 0x08) { // Check if this is the start of a packet
                mouse_cycle++;
            }
            break;
        case 1:
            mouse_byte[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = data;
            mouse_cycle = 0;
            
            // Update mouse state
            mouse_state.buttons = mouse_byte[0] & 0x07;
            
            // Handle X movement
            int8_t x = mouse_byte[1];
            if (mouse_byte[0] & MOUSE_X_SIGN) {
                x |= 0xFFFFFF00;
            }
            mouse_state.x += x;
            
            // Handle Y movement
            int8_t y = mouse_byte[2];
            if (mouse_byte[0] & MOUSE_Y_SIGN) {
                y |= 0xFFFFFF00;
            }
            mouse_state.y -= y; // Inverted Y axis
            
            // Clamp coordinates to screen boundaries
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.x >= SCREEN_WIDTH) mouse_state.x = SCREEN_WIDTH - 1;
            if (mouse_state.y >= SCREEN_HEIGHT) mouse_state.y = SCREEN_HEIGHT - 1;
            
            // Update cursor position
            update_cursor(mouse_state.x, mouse_state.y);
            
            // Call callback if registered
            if (mouse_callback) {
                mouse_callback(&mouse_state);
            }
            break;
    }
    
    pic_send_eoi(12);  // Send End of Interrupt
}

void mouse_init(void) {
    // Enable PS/2 mouse
    mouse_wait(1);
    outb(0x64, 0xA8);
    
    // Enable interrupts
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = inb(0x60) | 2;
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    
    // Use default settings
    mouse_write(0xF6);
    mouse_read();  // Acknowledge
    
    // Enable mouse
    mouse_write(0xF4);
    mouse_read();  // Acknowledge
    
    // Register interrupt handler
    register_interrupt_handler(44, mouse_handle_interrupt);
    pic_enable_irq(12);
}

void mouse_set_callback(mouse_callback_t callback) {
    mouse_callback = callback;
}

void mouse_get_state(mouse_state_t* state) {
    if (state) {
        state->x = mouse_state.x;
        state->y = mouse_state.y;
        state->buttons = mouse_state.buttons;
    }
}