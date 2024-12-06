#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include "isr.h"

// Mouse I/O ports
#define MOUSE_DATA_PORT    0x60
#define MOUSE_STATUS_PORT  0x64
#define MOUSE_COMMAND_PORT 0x64

// Mouse commands
#define MOUSE_WRITE       0xD4
#define MOUSE_READ        0x20
#define MOUSE_ENABLE_AUX  0xA8
#define MOUSE_GET_STATUS  0x20
#define MOUSE_SET_STATUS  0x60

// Mouse packet flags
#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_RIGHT_BUTTON  0x02
#define MOUSE_MIDDLE_BUTTON 0x04
#define MOUSE_X_SIGN       0x10
#define MOUSE_Y_SIGN       0x20
#define MOUSE_X_OVERFLOW   0x40
#define MOUSE_Y_OVERFLOW   0x80

// Mouse structure
typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;
} mouse_state_t;

// Function declarations
void mouse_init(void);
void mouse_wait(uint8_t type);
void mouse_write(uint8_t data);
uint8_t mouse_read(void);
void mouse_handle_interrupt(registers_t* regs);
void mouse_get_state(mouse_state_t* state);

#endif // MOUSE_H
