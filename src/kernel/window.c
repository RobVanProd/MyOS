#include "window.h"
#include "graphics.h"
#include "kheap.h"
#include <string.h>

// Window system state
static window_t* window_list = NULL;
static window_t* focused_window = NULL;

// Initialize window system
void window_system_init(void) {
    window_list = NULL;
    focused_window = NULL;
}

// Create a new window
window_t* create_window(int x, int y, int width, int height, const char* title, uint32_t flags) {
    window_t* window = kmalloc(sizeof(window_t));
    if (!window) return NULL;

    // Initialize window structure
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->flags = flags;
    strncpy(window->title, title, sizeof(window->title) - 1);
    window->title[sizeof(window->title) - 1] = '\0';
    
    // Allocate window buffer
    window->buffer = kmalloc(width * height * sizeof(uint32_t));
    if (!window->buffer) {
        kfree(window);
        return NULL;
    }
    
    // Clear window buffer
    memset(window->buffer, COLOR_WINDOW_BG, width * height * sizeof(uint32_t));
    
    // Initialize callbacks
    window->on_draw = NULL;
    window->on_key = NULL;
    window->on_click = NULL;
    window->data = NULL;
    
    // Add to window list
    window->next = window_list;
    window->prev = NULL;
    if (window_list) {
        window_list->prev = window;
    }
    window_list = window;
    
    // Set focus to new window
    focused_window = window;
    
    return window;
}

// Destroy a window
void destroy_window(window_t* window) {
    if (!window) return;
    
    // Remove from window list
    if (window->prev) {
        window->prev->next = window->next;
    } else {
        window_list = window->next;
    }
    if (window->next) {
        window->next->prev = window->prev;
    }
    
    // Update focus if needed
    if (focused_window == window) {
        focused_window = window_list;  // Focus first window
    }
    
    // Free window buffer
    if (window->buffer) {
        kfree(window->buffer);
    }
    
    // Free window structure
    kfree(window);
}

// Invalidate window (mark for redraw)
void window_invalidate(window_t* window) {
    if (!window) return;
    
    // Draw window background
    draw_rect(window->x, window->y, window->width, window->height, COLOR_WINDOW_BG);
    
    // Draw window border
    draw_rect(window->x, window->y, window->width, 1, COLOR_WINDOW_BORDER);
    draw_rect(window->x, window->y + window->height - 1, window->width, 1, COLOR_WINDOW_BORDER);
    draw_rect(window->x, window->y, 1, window->height, COLOR_WINDOW_BORDER);
    draw_rect(window->x + window->width - 1, window->y, 1, window->height, COLOR_WINDOW_BORDER);
    
    // Draw title bar if needed
    if (window->flags & WINDOW_HAS_TITLE) {
        draw_rect(window->x + 1, window->y + 1, window->width - 2, 20, COLOR_WINDOW_TITLE);
        // Draw title text
        int title_x = window->x + 5;
        int title_y = window->y + 6;
        for (const char* c = window->title; *c; c++) {
            draw_char(title_x, title_y, *c, COLOR_WINDOW_BG);
            title_x += 8;
        }
    }
    
    // Call window's draw callback if set
    if (window->on_draw) {
        window->on_draw(window);
    }
}

// Check if window has focus
bool window_has_focus(window_t* window) {
    return window == focused_window;
}

// Update window system
void window_system_update(void) {
    window_t* window = window_list;
    while (window) {
        if (window->flags & WINDOW_NEEDS_REDRAW) {
            // Draw window contents
            for (int y = 0; y < window->height; y++) {
                for (int x = 0; x < window->width; x++) {
                    int screen_x = window->x + x;
                    int screen_y = window->y + y;
                    if (screen_x >= 0 && screen_x < SCREEN_WIDTH &&
                        screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                        draw_pixel(screen_x, screen_y, window->buffer[y * window->width + x]);
                    }
                }
            }

            // Call window's draw handler if set
            if (window->on_draw) {
                window->on_draw(window);
            }

            window->flags &= ~WINDOW_NEEDS_REDRAW;
        }
        window = window->next;
    }
}

// Get currently focused window
window_t* window_get_focused(void) {
    return focused_window;
}

// Find window at screen coordinates
window_t* window_find_at(int x, int y) {
    window_t* window = window_list;
    while (window) {
        if (x >= window->x && x < window->x + window->width &&
            y >= window->y && y < window->y + window->height) {
            return window;
        }
        window = window->next;
    }
    return NULL;
}

// Bring window to front
void window_bring_to_front(window_t* window) {
    if (!window || window == window_list) return;

    // Remove from current position
    if (window->prev) window->prev->next = window->next;
    if (window->next) window->next->prev = window->prev;

    // Add to front
    window->prev = NULL;
    window->next = window_list;
    if (window_list) window_list->prev = window;
    window_list = window;

    // Update focus
    if (focused_window) focused_window->flags &= ~WINDOW_FLAG_FOCUSED;
    focused_window = window;
    window->flags |= WINDOW_FLAG_FOCUSED;
}

// Send window to back
void window_send_to_back(window_t* window) {
    if (!window || !window->next) return;

    // Remove from current position
    if (window->prev) window->prev->next = window->next;
    if (window == window_list) window_list = window->next;
    window->next->prev = window->prev;

    // Find last window
    window_t* last = window_list;
    while (last->next) last = last->next;

    // Add to back
    last->next = window;
    window->prev = last;
    window->next = NULL;

    // Update focus if needed
    if (window == focused_window) {
        focused_window = window_list;
        window->flags &= ~WINDOW_FLAG_FOCUSED;
        focused_window->flags |= WINDOW_FLAG_FOCUSED;
    }
}
