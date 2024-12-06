#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <graphics.h>

#define NOTEPAD_MAX_TEXT 4096
#define NOTEPAD_LINE_HEIGHT 16
#define NOTEPAD_PADDING 4

typedef struct {
    char text[NOTEPAD_MAX_TEXT];
    int cursor_pos;
    int scroll_y;
    window_t* window;
} notepad_t;

// Notepad functions
notepad_t* create_notepad(int x, int y);
void destroy_notepad(notepad_t* notepad);
void notepad_handle_key(window_t* window, char key);
void notepad_handle_click(window_t* window, int x, int y);
void notepad_draw(window_t* window);

#endif