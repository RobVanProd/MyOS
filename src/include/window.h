#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

// Window flags
#define WINDOW_MOVABLE    (1 << 0)
#define WINDOW_RESIZABLE  (1 << 1)
#define WINDOW_HAS_TITLE  (1 << 2)
#define WINDOW_HAS_CLOSE  (1 << 3)

// Forward declaration for event handlers
struct window;

// Window event handlers
typedef void (*window_key_handler)(struct window* window, char key);
typedef void (*window_click_handler)(struct window* window, int x, int y);
typedef void (*window_draw_handler)(struct window* window);

// Window structure
typedef struct window {
    int x, y;                  // Window position
    int width, height;         // Window dimensions
    uint32_t flags;           // Window flags
    char title[32];           // Window title
    uint8_t* buffer;          // Window buffer
    void* data;               // Window-specific data
    struct window* next;      // Next window in list
    
    // Event handlers
    window_key_handler on_key;
    window_click_handler on_click;
    window_draw_handler on_draw;
} window_t;

// Window functions
window_t* create_window(int x, int y, int width, int height, const char* title, uint32_t flags);
void destroy_window(window_t* window);
void window_invalidate(window_t* window);

#endif
