#include "notepad.h"
#include <memory.h>
#include <graphics.h>
#include <string.h>

notepad_t* create_notepad(int x, int y) {
    notepad_t* notepad = kmalloc(sizeof(notepad_t));
    if (!notepad) return NULL;
    
    // Initialize notepad data
    notepad->cursor_pos = 0;
    notepad->scroll_y = 0;
    notepad->text[0] = '\0';
    
    // Create window
    notepad->window = create_window(x, y, 400, 300, "Notepad", 
        WINDOW_MOVABLE | WINDOW_RESIZABLE | WINDOW_HAS_TITLE | WINDOW_HAS_CLOSE);
    
    if (!notepad->window) {
        kfree(notepad);
        return NULL;
    }
    
    // Set window callbacks
    notepad->window->on_key = notepad_handle_key;
    notepad->window->on_click = notepad_handle_click;
    notepad->window->on_draw = notepad_draw;
    
    return notepad;
}

void destroy_notepad(notepad_t* notepad) {
    if (!notepad) return;
    destroy_window(notepad->window);
    kfree(notepad);
}

void notepad_handle_key(window_t* window, char key) {
    notepad_t* notepad = (notepad_t*)window->data;
    
    // Handle backspace
    if (key == '\b') {
        if (notepad->cursor_pos > 0) {
            int i;
            for (i = notepad->cursor_pos - 1; notepad->text[i]; i++) {
                notepad->text[i] = notepad->text[i + 1];
            }
            notepad->cursor_pos--;
        }
    }
    // Handle regular characters
    else if (key >= 32 && key < 127) {
        if (notepad->cursor_pos < NOTEPAD_MAX_TEXT - 1) {
            // Shift characters right
            int i;
            for (i = NOTEPAD_MAX_TEXT - 2; i >= notepad->cursor_pos; i--) {
                notepad->text[i + 1] = notepad->text[i];
            }
            // Insert new character
            notepad->text[notepad->cursor_pos++] = key;
        }
    }
    
    // Request redraw
    window_invalidate(window);
}

void notepad_handle_click(window_t* window, int x, int y) {
    notepad_t* notepad = (notepad_t*)window->data;
    
    // Convert click coordinates to character position
    int char_width = 8;  // Assuming fixed-width font
    int char_height = 16;
    
    // Calculate character position from click coordinates
    int click_x = (x - 5) / char_width;  // 5 pixel margin
    int click_y = (y - 25 + notepad->scroll_y) / char_height;  // 25 pixel title bar
    
    // Find closest character position
    int pos = 0;
    int current_x = 0;
    int current_y = 0;
    
    for (pos = 0; notepad->text[pos]; pos++) {
        if (current_y == click_y && current_x == click_x) {
            break;
        }
        
        if (notepad->text[pos] == '\n') {
            current_y++;
            current_x = 0;
        } else {
            current_x++;
            if (current_x >= (window->width - 10) / char_width) {
                current_y++;
                current_x = 0;
            }
        }
    }
    
    notepad->cursor_pos = pos;
    window_invalidate(window);
}

void notepad_draw(window_t* window) {
    notepad_t* notepad = (notepad_t*)window->data;
    
    // Clear window
    draw_rect(window->x, window->y + 25, window->width, window->height - 25, COLOR_WINDOW_BG);
    
    // Draw text
    int x = window->x + 5;
    int y = window->y + 25 - notepad->scroll_y;
    int char_width = 8;
    int char_height = 16;
    
    for (int i = 0; notepad->text[i]; i++) {
        if (notepad->text[i] == '\n') {
            x = window->x + 5;
            y += char_height;
            continue;
        }
        
        if (x + char_width > window->x + window->width - 5) {
            x = window->x + 5;
            y += char_height;
        }
        
        draw_char(x, y, notepad->text[i], 0x000000);
        
        // Draw cursor
        if (i == notepad->cursor_pos) {
            draw_rect(x, y, 2, char_height, 0x000000);
        }
        
        x += char_width;
    }
    
    // Draw cursor at end if needed
    if (notepad->cursor_pos >= 0 && (size_t)notepad->cursor_pos == strlen(notepad->text)) {
        draw_rect(x, y, 2, char_height, 0x000000);
    }
}