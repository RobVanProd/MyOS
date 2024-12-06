#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>
#include "isr.h"
#include "pic.h"

// Keyboard ports
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

// Keyboard commands
#define KEYBOARD_CMD_SET_LEDS    0xED
#define KEYBOARD_CMD_ECHO        0xEE
#define KEYBOARD_CMD_GET_ID      0xF2
#define KEYBOARD_CMD_SET_RATE    0xF3
#define KEYBOARD_CMD_ENABLE      0xF4
#define KEYBOARD_CMD_DISABLE     0xF5
#define KEYBOARD_CMD_RESET       0xFF

// Special keys
#define KEY_ESCAPE      0x01
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F
#define KEY_ENTER       0x1C
#define KEY_CTRL        0x1D
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_PRINTSCREEN 0x37
#define KEY_ALT         0x38
#define KEY_CAPSLOCK    0x3A
#define KEY_F1          0x3B
#define KEY_F2          0x3C
#define KEY_F3          0x3D
#define KEY_F4          0x3E
#define KEY_F5          0x3F
#define KEY_F6          0x40
#define KEY_F7          0x41
#define KEY_F8          0x42
#define KEY_F9          0x43
#define KEY_F10         0x44
#define KEY_NUMLOCK     0x45
#define KEY_SCROLLLOCK  0x46
#define KEY_HOME        0x47
#define KEY_UP          0x48
#define KEY_PAGEUP      0x49
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D
#define KEY_END         0x4F
#define KEY_DOWN        0x50
#define KEY_PAGEDOWN    0x51
#define KEY_INSERT      0x52
#define KEY_DELETE      0x53
#define KEY_F11         0x57
#define KEY_F12         0x58

// Function declarations
void keyboard_init(void);
bool keyboard_buffer_empty(void);
char keyboard_getchar(void);
void keyboard_handler(registers_t* regs);
void keyboard_send_command(uint8_t command);
uint8_t keyboard_read_status(void);
uint8_t keyboard_status(void);
void keyboard_wait(void);
uint8_t keyboard_read_data(void);
void keyboard_write_command(uint8_t cmd);
void keyboard_write_data(uint8_t data);

// Keyboard state
extern uint8_t keyboard_scancode;
extern uint8_t keyboard_shift_pressed;
extern uint8_t keyboard_ctrl_pressed;
extern uint8_t keyboard_alt_pressed;
extern uint8_t keyboard_caps_lock;
extern uint8_t keyboard_num_lock;
extern uint8_t keyboard_scroll_lock;

#endif // KEYBOARD_H
