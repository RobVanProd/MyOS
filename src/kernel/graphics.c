#include "graphics.h"
#include "io.h"
#include "terminal.h"
#include "heap.h"
#include "window.h"
#include <string.h>

// VGA memory
static uint32_t* vga_memory = (uint32_t*)0xA0000;
static uint32_t* back_buffer = NULL;

// Simple 8x8 font data for basic characters
static const uint8_t font_8x8[128][8] = {
    // Space
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // Basic ASCII characters
    // ... Add more characters as needed
};

void graphics_init(void) {
    // Initialize back buffer
    back_buffer = heap_alloc(320 * 200 * sizeof(uint32_t));
    if (back_buffer) {
        memset(back_buffer, 0, 320 * 200 * sizeof(uint32_t));
    }
}

void draw_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= 320 || y < 0 || y >= 200) return;
    back_buffer[y * 320 + x] = color;
}

void draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    while (1) {
        draw_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 < dy) { err += dx; y1 += sy; }
    }
}

void draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            draw_pixel(x + j, y + i, color);
        }
    }
}

void draw_char(int x, int y, char c, uint32_t color) {
    if (c < 0 || c >= 128) return;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (font_8x8[c][i] & (1 << j)) {
                draw_pixel(x + j, y + i, color);
            }
        }
    }
}

void clear_screen(uint32_t color) {
    for (int i = 0; i < 320 * 200; i++) {
        back_buffer[i] = color;
    }
}