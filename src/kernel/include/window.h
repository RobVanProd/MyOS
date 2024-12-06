#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>
#include <stdbool.h>
#include "font.h"
#include "kheap.h"

// Window flags
#define WINDOW_MOVABLE      0x01
#define WINDOW_RESIZABLE    0x02
#define WINDOW_HAS_TITLE    0x04
#define WINDOW_HAS_CLOSE    0x08
#define WINDOW_MINIMIZABLE  0x10
#define WINDOW_MAXIMIZABLE  0x20
#define WINDOW_FLAG_FOCUSED 0x80
#define WINDOW_NEEDS_REDRAW 0x100

// Window structure
typedef struct window {
    int x;
    int y;
    int width;
    int height;
    uint32_t flags;
    char title[64];
    uint32_t* buffer;
    struct window* next;
    struct window* prev;
    void (*on_draw)(struct window*);
    void (*on_key)(struct window*, int);
    void (*on_click)(struct window*, int, int, int);
    void* data;
} window_t;

// Window system functions
void window_system_init(void);
void window_system_update(void);
window_t* window_get_focused(void);
window_t* window_find_at(int x, int y);
void window_bring_to_front(window_t* window);
void window_send_to_back(window_t* window);

// Window management functions
window_t* create_window(int x, int y, int width, int height, const char* title, uint32_t flags);
void destroy_window(window_t* window);
void window_invalidate(window_t* window);
bool window_has_focus(window_t* window);

#endif /* WINDOW_H */
