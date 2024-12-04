#include "notepad.h"
#include "../kernel/memory.h"
#include "../kernel/graphics.h"

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
    notepad_t* notepad = (notepad_t*)window;
    
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
            // Shift text right
            int i;
            for (i = NOTEPAD_MAX_TEXT - 2; i >= notepad->cursor_pos; i--) {
                notepad->text[i + 1] = notepad->text[i];
            }
            // Insert new character
            notepad->text[notepad->cursor_pos] = key;
            notepad->cursor_pos++;
        }
    }
}

void notepad_handle_click(window_t* window, int x, int y) {
    notepad_t* notepad = (notepad_t*)window;
    
    // Calculate cursor position from click coordinates
    int text_y = y - NOTEPAD_PADDING - 16; // Subtract title bar height
    int text_x = x - NOTEPAD_PADDING;
    
    if (text_y >= 0) {
        int line = text_y / NOTEPAD_LINE_HEIGHT;
        int col = text_x / 8; // Assuming 8 pixel wide font
        
        // Find the actual position in text
        int pos = 0;
        int current_line = 0;
        
        while (notepad->text[pos] && current_line < line) {
            if (notepad->text[pos] == '\n') current_line++;
            pos++;
        }
        
        // Add column offset
        pos += col;
        
        // Clamp cursor position
        if (pos < 0) pos = 0;
        if (pos > strlen(notepad->text)) pos = strlen(notepad->text);
        
        notepad->cursor_pos = pos;
    }
}

void notepad_draw(window_t* window) {
    notepad_t* notepad = (notepad_t*)window;
    int x = NOTEPAD_PADDING;
    int y = NOTEPAD_PADDING + 16; // Start below title bar
    int line_start = 0;
    int i;
    
    // Draw text
    for (i = 0; notepad->text[i]; i++) {
        if (notepad->text[i] == '\n' || x >= window->width - NOTEPAD_PADDING) {
            x = NOTEPAD_PADDING;
            y += NOTEPAD_LINE_HEIGHT;
            if (notepad->text[i] == '\n') {
                line_start = i + 1;
                continue;
            }
        }
        
        draw_char(window->x + x, window->y + y, notepad->text[i], COLOR_TEXT);
        x += 8; // Assuming 8 pixel wide font
        
        // Draw cursor
        if (i == notepad->cursor_pos) {
            draw_rect(window->x + x - 8, window->y + y, 2, NOTEPAD_LINE_HEIGHT, COLOR_TEXT);
        }
    }
    
    // Draw cursor at end of text if needed
    if (i == notepad->cursor_pos) {
        draw_rect(window->x + x, window->y + y, 2, NOTEPAD_LINE_HEIGHT, COLOR_TEXT);
    }
} 