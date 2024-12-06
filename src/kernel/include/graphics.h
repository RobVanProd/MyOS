#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

// Screen dimensions (VGA mode 13h)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// Colors
#define COLOR_BLACK   0x000000
#define COLOR_WHITE   0xFFFFFF
#define COLOR_RED     0xFF0000
#define COLOR_GREEN   0x00FF00
#define COLOR_BLUE    0x0000FF

// Default colors
#define COLOR_BACKGROUND COLOR_BLACK
#define COLOR_FOREGROUND COLOR_WHITE
#define COLOR_WINDOW_BG COLOR_WHITE
#define COLOR_WINDOW_TITLE COLOR_BLACK
#define COLOR_WINDOW_BORDER COLOR_BLACK

// Graphics functions
void graphics_init(void);
void draw_pixel(int x, int y, uint32_t color);
void draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void draw_char(int x, int y, char c, uint32_t color);
void clear_screen(uint32_t color);

#endif /* GRAPHICS_H */
