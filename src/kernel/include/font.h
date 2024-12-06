#ifndef FONT_H
#define FONT_H

#include <stdint.h>

// Font dimensions
#define FONT_WIDTH  8
#define FONT_HEIGHT 16

// Get font data for a character
const uint8_t* font_get_glyph(char c);

#endif /* FONT_H */
