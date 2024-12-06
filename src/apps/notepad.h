#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <window.h>

// Maximum text size for notepad
#define NOTEPAD_MAX_TEXT 4096

// Notepad structure
typedef struct {
    window_t* window;       // Window handle
    char text[NOTEPAD_MAX_TEXT];  // Text buffer
    int cursor_pos;        // Current cursor position
    int scroll_y;          // Vertical scroll position
} notepad_t;

// Notepad functions
notepad_t* create_notepad(int x, int y);
void destroy_notepad(notepad_t* notepad);
void notepad_handle_key(struct window* window, int key);
void notepad_handle_click(struct window* window, int x, int y, int button);
void notepad_draw(struct window* window);

#endif
