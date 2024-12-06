#include "notepad.h"
#include <heap.h>
#include <graphics.h>
#include <string.h>

notepad_t* create_notepad(int x, int y) {
    notepad_t* notepad = heap_alloc(sizeof(notepad_t));
    if (!notepad) return NULL;
    
    // Initialize notepad data
    notepad->cursor_pos = 0;
    notepad->scroll_y = 0;
    notepad->text[0] = '\0';
    
    // Create window
    notepad->window = create_window(x, y, 400, 300, "Notepad", 
        WINDOW_MOVABLE | WINDOW_RESIZABLE | WINDOW_HAS_TITLE | WINDOW_HAS_CLOSE);
    
    if (!notepad->window) {
        heap_free(notepad);
        return NULL;
    }
    
    // Set window callbacks
    notepad->window->on_key = notepad_handle_key;
    notepad->window->on_click = notepad_handle_click;
    notepad->window->on_draw = notepad_draw;
    notepad->window->data = notepad;
    
    return notepad;
}

void destroy_notepad(notepad_t* notepad) {
    if (!notepad) return;
    destroy_window(notepad->window);
    heap_free(notepad);
}

void notepad_handle_key(struct window* window, int key) {
    notepad_t* notepad = (notepad_t*)window->data;
    
    // Handle backspace
    if (key == '\b') {
        if (notepad->cursor_pos > 0) {
            int i;
            for (i = notepad->cursor_pos - 1; notepad->text[i]; i++) {
                notepad->text[i] = notepad->text[i + 1];
            }
            notepad->cursor_pos--;
            window_invalidate(window);
        }
    }
    // Handle regular characters
    else if (key >= 32 && key <= 126) {
        if (notepad->cursor_pos < NOTEPAD_MAX_TEXT - 1) {
            // Make room for new character
            for (int i = NOTEPAD_MAX_TEXT - 1; i > notepad->cursor_pos; i--) {
                notepad->text[i] = notepad->text[i - 1];
            }
            notepad->text[notepad->cursor_pos++] = key;
            window_invalidate(window);
        }
    }
}

void notepad_handle_click(struct window* window, int x, int y, int button) {
    notepad_t* notepad = (notepad_t*)window->data;
    
    // Convert click coordinates to text position
    int text_x = (x - 5) / 8;
    int text_y = (y - 25) / 16;
    
    // Calculate new cursor position
    int pos = 0;
    int line = 0;
    int col = 0;
    
    while (notepad->text[pos]) {
        if (line == text_y && col == text_x) {
            break;
        }
        if (notepad->text[pos] == '\n' || col >= 48) {
            line++;
            col = 0;
        } else {
            col++;
        }
        pos++;
    }
    
    notepad->cursor_pos = pos;
    window_invalidate(window);
}

void notepad_draw(struct window* window) {
    notepad_t* notepad = (notepad_t*)window->data;
    
    // Draw window background
    draw_rect(window->x, window->y, window->width, window->height, COLOR_WINDOW_BG);
    
    // Draw text area background
    draw_rect(window->x + 5, window->y + 25, 
             window->width - 10, window->height - 30, COLOR_WHITE);
    
    // Draw text
    int x = 5;
    int y = 25;
    int pos = 0;
    
    while (notepad->text[pos]) {
        if (notepad->text[pos] == '\n' || x >= window->width - 13) {
            x = 5;
            y += 16;
            if (notepad->text[pos] == '\n') {
                pos++;
                continue;
            }
        }
        
        if (y >= window->height - 5) break;
        
        draw_char(window->x + x, window->y + y, notepad->text[pos], COLOR_BLACK);
        x += 8;
        pos++;
    }
    
    // Draw cursor if window has focus
    if (window_has_focus(window)) {
        int cursor_x = 5;
        int cursor_y = 25;
        pos = 0;
        
        while (pos < notepad->cursor_pos) {
            if (notepad->text[pos] == '\n' || cursor_x >= window->width - 13) {
                cursor_x = 5;
                cursor_y += 16;
                if (notepad->text[pos] == '\n') {
                    pos++;
                    continue;
                }
            }
            cursor_x += 8;
            pos++;
        }
        
        draw_rect(window->x + cursor_x, window->y + cursor_y, 
                 2, 14, COLOR_BLACK);
    }
}