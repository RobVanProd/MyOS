#include "cursor.h"
#include "io.h"

// VGA registers
#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5

// VGA cursor registers
#define VGA_CURSOR_START 0x0A
#define VGA_CURSOR_END 0x0B
#define VGA_CURSOR_HIGH 0x0E
#define VGA_CURSOR_LOW 0x0F

// Screen dimensions
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Update hardware cursor position
void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    // Write high byte
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_HIGH);
    outb(VGA_DATA_REGISTER, (pos >> 8) & 0xFF);
    
    // Write low byte
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOW);
    outb(VGA_DATA_REGISTER, pos & 0xFF);
}

// Enable hardware cursor
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    // Set cursor start line
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_START);
    outb(VGA_DATA_REGISTER, (inb(VGA_DATA_REGISTER) & 0xC0) | cursor_start);
    
    // Set cursor end line
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_END);
    outb(VGA_DATA_REGISTER, (inb(VGA_DATA_REGISTER) & 0xE0) | cursor_end);
}

// Disable hardware cursor
void disable_cursor(void) {
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_START);
    outb(VGA_DATA_REGISTER, 0x20);
}

// Get current cursor position
void get_cursor_position(int* x, int* y) {
    uint16_t pos = 0;
    
    // Read high byte
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_HIGH);
    pos = inb(VGA_DATA_REGISTER) << 8;
    
    // Read low byte
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOW);
    pos |= inb(VGA_DATA_REGISTER);
    
    // Convert to x,y coordinates
    *x = pos % VGA_WIDTH;
    *y = pos / VGA_WIDTH;
}
