#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "terminal.h"

// Keyboard buffer size
#define KEYBOARD_BUFFER_SIZE 256

// Keyboard buffer
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_start = 0;
static int buffer_end = 0;

// Shift state
static int shift_pressed = 0;
static int caps_lock = 0;

// Scancode to ASCII conversion tables
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

// Add a character to the keyboard buffer
static void keyboard_buffer_put(char c) {
    int next_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_end != buffer_start) {
        keyboard_buffer[buffer_end] = c;
        buffer_end = next_end;
    }
}

void keyboard_init(void) {
    // Enable keyboard IRQ
    pic_enable_irq(1);
    
    // Reset keyboard and wait for ACK
    outb(KEYBOARD_COMMAND_PORT, KEYBOARD_CMD_RESET);
    while ((inb(KEYBOARD_STATUS_PORT) & 1) == 0);
    if (inb(KEYBOARD_DATA_PORT) == 0xFA) {
        // Enable scanning
        outb(KEYBOARD_DATA_PORT, KEYBOARD_CMD_ENABLE);
    }
}

void keyboard_handler(registers_t* regs) {
    uint8_t scancode = keyboard_read_data();

    switch(scancode) {
        case KEY_CAPSLOCK:
            // Handle caps lock
            caps_lock = !caps_lock;
            break;
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            shift_pressed = 1;
            break;
        case KEY_LSHIFT | 0x80:  // Key release
        case KEY_RSHIFT | 0x80:
            shift_pressed = 0;
            break;
        default:
            // Convert scancode to ASCII if it's a press (not a release)
            if (!(scancode & 0x80) && scancode < sizeof(scancode_to_ascii)) {
                char ascii;
                if (shift_pressed) {
                    ascii = scancode_to_ascii_shift[scancode];
                } else {
                    ascii = scancode_to_ascii[scancode];
                }

                // Handle caps lock
                if (caps_lock && ascii >= 'a' && ascii <= 'z') {
                    ascii -= 32;  // Convert to uppercase
                }

                if (ascii) {
                    keyboard_buffer_put(ascii);
                    // Echo character to screen
                    terminal_putchar(ascii);
                }
            }
            break;
    }

    // Send EOI to PIC
    pic_send_eoi(1);
}

char keyboard_getchar(void) {
    char c = 0;
    if (buffer_start != buffer_end) {
        c = keyboard_buffer[buffer_start];
        buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    }
    return c;
}

int keyboard_haskey(void) {
    return buffer_start != buffer_end;
}