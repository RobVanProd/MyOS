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

bool keyboard_buffer_empty(void) {
    return buffer_start == buffer_end;
}

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

static const char scancode_to_ascii_shift[] __attribute__((unused)) = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

// Wait for keyboard controller to be ready
void keyboard_wait(void) {
    while ((inb(KEYBOARD_STATUS_PORT) & 2) != 0) {
        // Wait for keyboard controller input buffer to be empty
    }
}

// Read data from keyboard
uint8_t keyboard_read_data(void) {
    keyboard_wait();
    return inb(KEYBOARD_DATA_PORT);
}

// Write command to keyboard
void keyboard_write_command(uint8_t command) {
    keyboard_wait();
    outb(KEYBOARD_COMMAND_PORT, command);
}

// Write data to keyboard
void keyboard_write_data(uint8_t data) {
    keyboard_wait();
    outb(KEYBOARD_DATA_PORT, data);
}

// Read keyboard status
uint8_t keyboard_status(void) {
    return inb(KEYBOARD_STATUS_PORT);
}

// Add a character to the keyboard buffer
static void keyboard_buffer_put(char c) __attribute__((unused));
static void keyboard_buffer_put(char c) {
    int next_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_end != buffer_start) {
        keyboard_buffer[buffer_end] = c;
        buffer_end = next_end;
    }
}

char keyboard_getchar(void) {
    if (keyboard_buffer_empty()) {
        return 0;
    }
    char c = keyboard_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

void keyboard_init(void) {
    // Enable keyboard IRQ
    pic_enable_irq(1);
    
    // Reset keyboard and wait for ACK
    keyboard_write_command(KEYBOARD_CMD_RESET);
    while ((keyboard_status() & 1) == 0);
    if (keyboard_read_data() == 0xFA) {
        // Enable scanning
        keyboard_write_command(KEYBOARD_CMD_ENABLE);
    }
}

void keyboard_handler(registers_t* regs) {
    (void)regs;  // Unused parameter
    
    uint8_t scancode = keyboard_read_data();
    
    // Handle shift keys
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    }
    
    // Handle caps lock
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
    }
    
    // Only handle key press events (not key release)
    if (!(scancode & 0x80)) {
        char c = scancode_to_ascii[scancode];
        if (c) {
            // Add to buffer if there's space
            int next_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
            if (next_end != buffer_start) {
                keyboard_buffer[buffer_end] = c;
                buffer_end = next_end;
            }
        }
    }
    
    pic_send_eoi(1);
}

int keyboard_haskey(void) {
    return buffer_start != buffer_end;
}