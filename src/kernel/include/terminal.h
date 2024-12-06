#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// VGA hardware text mode color constants
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Terminal functions
void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_writehex(uint32_t value);
void terminal_writedec(uint32_t value);
void terminal_clear(void);
void terminal_scroll(void);
void terminal_newline(void);

// Terminal input functions
char terminal_getchar(void);

// Printf-like functions
int kprintf(const char* format, ...);
int kvprintf(const char* format, va_list args);

#endif /* TERMINAL_H */
