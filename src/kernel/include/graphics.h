#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

// Screen dimensions (VGA mode 13h)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// Color definitions
#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHT_GREY 7
#define COLOR_DARK_GREY 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_BROWN 14
#define COLOR_WHITE 15

// Default colors
#define COLOR_BACKGROUND COLOR_BLACK
#define COLOR_FOREGROUND COLOR_WHITE

// Window flags
#define WINDOW_MOVABLE     0x01
#define WINDOW_RESIZABLE   0x02
#define WINDOW_CLOSABLE    0x04
#define WINDOW_MINIMIZABLE 0x08
#define WINDOW_TITLEBAR    0x10

// Window structure
typedef struct window {
    int x, y;
    int width, height;
    char* title;
    uint32_t flags;
    uint8_t* buffer;
    struct window* next;
    struct window* prev;
} window_t;

// Function declarations
void graphics_init(void);
void set_pixel(int x, int y, uint8_t color);
void draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void draw_rect(int x, int y, int width, int height, uint8_t color);
void fill_rect(int x, int y, int width, int height, uint8_t color);
void draw_char(int x, int y, char c, uint8_t color);
void draw_string(int x, int y, const char* str, uint8_t color);
void draw_string_with_bg(int x, int y, const char* str, uint8_t fg_color, uint8_t bg_color);

// Window management
window_t create_window(int x, int y, int width, int height, const char* title, uint32_t flags);
void destroy_window(window_t* window);
void draw_window(window_t* window);
void bring_to_front(window_t* window);
void handle_window_click(int x, int y);
void handle_window_key(char key);

// Buffer management
void swap_buffers(void);
void clear_screen(uint8_t color);

// Mouse cursor
void draw_cursor(int x, int y);
void update_cursor(int x, int y);

#endif // GRAPHICS_H
