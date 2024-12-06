 #ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <stdbool.h>

// Mouse button flags
#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_RIGHT_BUTTON  0x02
#define MOUSE_MIDDLE_BUTTON 0x04

// Mouse event structure
typedef struct {
    int x;          // X position
    int y;          // Y position
    uint8_t buttons;  // Button state
    int8_t dx;       // X movement
    int8_t dy;       // Y movement
} mouse_event_t;

// Mouse callback function type
typedef void (*mouse_callback_t)(mouse_event_t*);

// Mouse functions
void mouse_init(void);
void mouse_set_callback(mouse_callback_t callback);
void mouse_handle_interrupt(void);
void mouse_update_position(int dx, int dy);

#endif