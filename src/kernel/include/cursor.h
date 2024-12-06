#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>

// Cursor functions
void update_cursor(int x, int y);
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void disable_cursor(void);
void get_cursor_position(int* x, int* y);

#endif /* CURSOR_H */
