#include "terminal.h"
#include "keyboard.h"
#include "process.h"
#include "string.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;
    
    // Clear the screen
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            const uint8_t uc = (uint8_t) (terminal_buffer[index] & 0xFF);
            terminal_buffer[index] = vga_entry(uc, color);
        }
    }
}

void terminal_scroll(void) {
    // Move all lines up by one
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t to_index = (y - 1) * VGA_WIDTH + x;
            const size_t from_index = y * VGA_WIDTH + x;
            terminal_buffer[to_index] = terminal_buffer[from_index];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_newline(void) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_newline();
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++)
        terminal_putchar(data[i]);
}

void terminal_writehex(uint32_t value) {
    char hex_str[11];  // "0x" + 8 hex digits + null terminator
    hex_str[0] = '0';
    hex_str[1] = 'x';
    hex_str[10] = '\0';

    // Convert each nibble to hex
    for (int i = 0; i < 8; i++) {
        int nibble = (value >> ((7 - i) * 4)) & 0xF;
        hex_str[i + 2] = nibble < 10 ? '0' + nibble : 'A' + (nibble - 10);
    }

    terminal_writestring(hex_str);
}

void terminal_writedec(uint32_t value) {
    char dec_str[11];  // Maximum 10 digits + null terminator
    int i = 0;
    
    // Handle special case for 0
    if (value == 0) {
        terminal_putchar('0');
        return;
    }
    
    // Convert to decimal string (in reverse)
    while (value > 0) {
        dec_str[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Print in correct order
    while (--i >= 0) {
        terminal_putchar(dec_str[i]);
    }
}

char terminal_getchar(void) {
    // Wait for a key from the keyboard buffer
    while (keyboard_buffer_empty()) {
        // Sleep or yield to other processes
        process_yield();
    }
    return keyboard_getchar();
}

int kvprintf(const char* format, va_list args) {
    int written = 0;
    
    while (*format != '\0') {
        if (*format != '%') {
            terminal_putchar(*format);
            written++;
            format++;
            continue;
        }
        
        format++; // Skip '%'
        
        switch (*format) {
            case 'c': {
                char c = (char)va_arg(args, int);
                terminal_putchar(c);
                written++;
                break;
            }
            case 's': {
                const char* str = va_arg(args, const char*);
                if (!str) str = "(null)";
                terminal_writestring(str);
                written += strlen(str);
                break;
            }
            case 'd':
            case 'i': {
                int num = va_arg(args, int);
                if (num < 0) {
                    terminal_putchar('-');
                    written++;
                    num = -num;
                }
                terminal_writedec((uint32_t)num);
                // Approximate number of digits
                int temp = num;
                do {
                    written++;
                    temp /= 10;
                } while (temp > 0);
                break;
            }
            case 'x': {
                uint32_t num = va_arg(args, uint32_t);
                terminal_writehex(num);
                written += 10; // "0x" + 8 hex digits
                break;
            }
            case '%': {
                terminal_putchar('%');
                written++;
                break;
            }
            default:
                terminal_putchar('%');
                terminal_putchar(*format);
                written += 2;
                break;
        }
        format++;
    }
    
    return written;
}

int kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = kvprintf(format, args);
    va_end(args);
    return ret;
}