#include "graphics.h"
#include "io.h"
#include "terminal.h"

// VGA memory
static uint8_t* vga_memory = (uint8_t*)0xA0000;
static uint8_t* back_buffer = NULL;

// Window management
static window_t* window_list = NULL;
static window_t* active_window = NULL;

// Mouse cursor position
static int cursor_x = 0;
static int cursor_y = 0;

// 8x8 font data
static const uint8_t font_8x8[128][8] = {
    // Basic ASCII font data (0-127)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    // ... Add more characters here
};

void graphics_init(void) {
    // Set VGA mode 13h (320x200, 256 colors)
    outb(0x3C8, 0);
    
    // Set up color palette
    for (int i = 0; i < 256; i++) {
        outb(0x3C9, (i >> 5) * 63 / 7);     // Red
        outb(0x3C9, ((i >> 2) & 7) * 63 / 7); // Green
        outb(0x3C9, (i & 3) * 63 / 3);      // Blue
    }
    
    // Allocate back buffer
    back_buffer = kmalloc(SCREEN_WIDTH * SCREEN_HEIGHT);
    
    // Clear screen
    clear_screen(COLOR_BACKGROUND);
}

void set_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        back_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

void draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    
    while (1) {
        set_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = err;
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 < dy) { err += dx; y1 += sy; }
    }
}

void draw_rect(int x, int y, int width, int height, uint8_t color) {
    draw_line(x, y, x + width - 1, y, color);
    draw_line(x + width - 1, y, x + width - 1, y + height - 1, color);
    draw_line(x, y + height - 1, x + width - 1, y + height - 1, color);
    draw_line(x, y, x, y + height - 1, color);
}

void fill_rect(int x, int y, int width, int height, uint8_t color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            set_pixel(x + j, y + i, color);
        }
    }
}

void draw_char(int x, int y, char c, uint8_t color) {
    if (c < 0 || c >= 128) return;
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (font_8x8[c][i] & (1 << j)) {
                set_pixel(x + j, y + i, color);
            }
        }
    }
}

void draw_string(int x, int y, const char* str, uint8_t color) {
    int pos_x = x;
    while (*str) {
        draw_char(pos_x, y, *str, color);
        pos_x += 8;
        str++;
    }
}

void draw_string_with_bg(int x, int y, const char* str, uint8_t fg_color, uint8_t bg_color) {
    int pos_x = x;
    while (*str) {
        fill_rect(pos_x, y, 8, 8, bg_color);
        draw_char(pos_x, y, *str, fg_color);
        pos_x += 8;
        str++;
    }
}

window_t* create_window(int x, int y, int width, int height, const char* title, uint8_t flags) {
    window_t* window = kmalloc(sizeof(window_t));
    if (!window) return NULL;
    
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->flags = flags;
    window->buffer = kmalloc(width * height);
    window->title = NULL;
    
    if (title) {
        int len = 0;
        while (title[len]) len++;
        window->title = kmalloc(len + 1);
        for (int i = 0; i <= len; i++) {
            window->title[i] = title[i];
        }
    }
    
    // Add to window list
    window->next = window_list;
    window_list = window;
    active_window = window;
    
    return window;
}

void destroy_window(window_t* window) {
    if (!window) return;
    
    // Remove from window list
    if (window_list == window) {
        window_list = window->next;
    } else {
        window_t* prev = window_list;
        while (prev && prev->next != window) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = window->next;
        }
    }
    
    if (active_window == window) {
        active_window = window_list;
    }
    
    // Free resources
    if (window->buffer) kfree(window->buffer);
    if (window->title) kfree(window->title);
    kfree(window);
}

void draw_window(window_t* window) {
    if (!window) return;
    
    // Draw window frame
    fill_rect(window->x, window->y, window->width, window->height, COLOR_WINDOW_BG);
    draw_rect(window->x, window->y, window->width, window->height, COLOR_WINDOW_FRAME);
    
    // Draw title bar if needed
    if (window->flags & WINDOW_HAS_TITLE && window->title) {
        fill_rect(window->x, window->y, window->width, 16, COLOR_WINDOW_FRAME);
        draw_string_with_bg(window->x + 4, window->y + 4, window->title, COLOR_TEXT, COLOR_WINDOW_FRAME);
        
        // Draw close button
        if (window->flags & WINDOW_HAS_CLOSE) {
            fill_rect(window->x + window->width - 16, window->y + 4, 8, 8, COLOR_RED);
        }
    }
    
    // Draw window contents
    if (window->on_draw) {
        window->on_draw(window);
    }
}

void bring_to_front(window_t* window) {
    if (!window || window == window_list) return;
    
    // Remove window from list
    window_t* prev = window_list;
    while (prev && prev->next != window) {
        prev = prev->next;
    }
    if (!prev) return;
    
    // Move to front
    prev->next = window->next;
    window->next = window_list;
    window_list = window;
    active_window = window;
}

void handle_window_click(int x, int y) {
    window_t* window = window_list;
    while (window) {
        if (x >= window->x && x < window->x + window->width &&
            y >= window->y && y < window->y + window->height) {
            // Check if clicking close button
            if ((window->flags & WINDOW_HAS_CLOSE) &&
                x >= window->x + window->width - 16 &&
                x < window->x + window->width - 8 &&
                y >= window->y + 4 &&
                y < window->y + 12) {
                destroy_window(window);
                return;
            }
            
            // Bring window to front
            bring_to_front(window);
            
            // Handle click in window
            if (window->on_click) {
                window->on_click(window, x - window->x, y - window->y);
            }
            return;
        }
        window = window->next;
    }
}

void handle_window_key(char key) {
    if (active_window && active_window->on_key) {
        active_window->on_key(active_window, key);
    }
}

void swap_buffers(void) {
    // Copy back buffer to VGA memory
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        vga_memory[i] = back_buffer[i];
    }
}

void clear_screen(uint8_t color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        back_buffer[i] = color;
    }
}

void draw_cursor(int x, int y) {
    // Draw a simple arrow cursor
    uint8_t cursor_color = COLOR_WHITE;
    set_pixel(x, y, cursor_color);
    set_pixel(x+1, y+1, cursor_color);
    set_pixel(x+2, y+2, cursor_color);
    set_pixel(x+3, y+3, cursor_color);
    set_pixel(x+1, y, cursor_color);
    set_pixel(x+2, y, cursor_color);
    set_pixel(x, y+1, cursor_color);
    set_pixel(x, y+2, cursor_color);
}

void update_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
} 