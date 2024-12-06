 #ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <isr.h>

// Mouse event structure
typedef struct {
    int x;      // X position
    int y;      // Y position
    int dx;     // X movement
    int dy;     // Y movement
    uint8_t buttons;  // Button state
} mouse_event_t;

// Mouse callback function type
typedef void (*mouse_callback_t)(mouse_event_t* event);

// Mouse functions
void mouse_init(void);
void mouse_set_callback(mouse_callback_t callback);
void mouse_handle_interrupt(registers_t* regs);

#endif