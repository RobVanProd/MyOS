#include "font.h"

// 8x16 font data - Basic ASCII characters (32-127)
// Each character is 16 bytes, representing 8x16 pixels
// 1 bit per pixel, MSB first
static const uint8_t font_data[96][16] = {
    // Space (32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (33)
    {0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
     0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00},
    // " (34)
    {0x00, 0x6C, 0x6C, 0x6C, 0x6C, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // # (35)
    {0x00, 0x36, 0x36, 0x7F, 0x36, 0x36, 0x36, 0x7F,
     0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // More characters...
    // We'll add more as needed, for now just implementing basic ones
};

// Get font data for a character
const uint8_t* font_get_glyph(char c) {
    // Only handle printable ASCII characters
    if (c < 32 || c > 127) {
        c = '?'; // Use question mark for invalid characters
    }
    
    // Return pointer to font data for character
    return font_data[c - 32];
}
