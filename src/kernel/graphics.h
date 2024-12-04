#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// Basic colors
#define COLOR_BLACK   0x00
#define COLOR_BLUE    0x01
#define COLOR_GREEN   0x02
#define COLOR_CYAN    0x03
#define COLOR_RED     0x04
#define COLOR_MAGENTA 0x05
#define COLOR_BROWN   0x06
#define COLOR_WHITE   0x0F
#define COLOR_GRAY    0x08

// GUI Colors
#define COLOR_BACKGROUND    0x10
#define COLOR_WINDOW_BG    0x17
#define COLOR_WINDOW_FRAME 0x1F
#define COLOR_TEXT         0x0F
#define COLOR_HIGHLIGHT    0x1E

// Window flags
#define WINDOW_MOVABLE     0x01
#define WINDOW_RESIZABLE   0x02
#define WINDOW_HAS_TITLE   0x04
#define WINDOW_HAS_CLOSE   0x08

// Basic shapes and drawing
void graphics_init(void);
void set_pixel(int x, int y, uint8_t color);
void draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void draw_rect(int x, int y, int width, int height, uint8_t color);
void fill_rect(int x, int y, int width, int height, uint8_t color);
void draw_circle(int x, int y, int radius, uint8_t color);
void fill_circle(int x, int y, int radius, uint8_t color);

// Text rendering
void draw_char(int x, int y, char c, uint8_t color);
void draw_string(int x, int y, const char* str, uint8_t color);
void draw_string_with_bg(int x, int y, const char* str, uint8_t fg_color, uint8_t bg_color);

// Window management
typedef struct window {
    int x, y;                  // Window position
    int width, height;         // Window dimensions
    uint8_t flags;            // Window flags
    char* title;              // Window title
    uint8_t* buffer;          // Window buffer
    struct window* next;      // Next window in list
    void (*on_click)(struct window*, int, int);    // Click handler
    void (*on_key)(struct window*, char);          // Key handler
    void (*on_draw)(struct window*);               // Draw handler
} window_t;

// Window management functions
window_t* create_window(int x, int y, int width, int height, const char* title, uint8_t flags);
void destroy_window(window_t* window);
void draw_window(window_t* window);
void bring_to_front(window_t* window);
void handle_window_click(int x, int y);
void handle_window_key(char key);

// Double buffering
void swap_buffers(void);
void clear_screen(uint8_t color);

// Mouse cursor
void draw_cursor(int x, int y);
void update_cursor(int x, int y);

// Effects and animations
void fade_in(window_t* window, int duration_ms);
void fade_out(window_t* window, int duration_ms);
void slide_in(window_t* window, int from_x, int from_y, int duration_ms);

#endif 