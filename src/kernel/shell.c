#include "shell.h"
#include "terminal.h"
#include "keyboard.h"
#include "command.h"
#include <string.h>

#define SHELL_PROMPT "$ "
#define MAX_INPUT_LENGTH 256

// Shell state
static bool shell_active = false;
static char input_buffer[MAX_INPUT_LENGTH];
static int input_position = 0;

// Initialize shell
void shell_init(void) {
    shell_active = false;
    input_position = 0;
    memset(input_buffer, 0, sizeof(input_buffer));
}

// Create a new shell
void create_shell(void) {
    shell_active = true;
    input_position = 0;
    memset(input_buffer, 0, sizeof(input_buffer));
    terminal_writestring(SHELL_PROMPT);
}

// Handle shell input
void shell_input(char c) {
    if (!shell_active) return;

    if (c == '\n') {
        terminal_writestring("\n");
        if (input_position > 0) {
            input_buffer[input_position] = '\0';
            command_execute(input_buffer);
            input_position = 0;
            memset(input_buffer, 0, sizeof(input_buffer));
        }
        terminal_writestring(SHELL_PROMPT);
    }
    else if (c == '\b') {
        if (input_position > 0) {
            input_position--;
            input_buffer[input_position] = '\0';
            terminal_writestring("\b \b");
        }
    }
    else if (input_position < MAX_INPUT_LENGTH - 1) {
        input_buffer[input_position++] = c;
        terminal_putchar(c);
    }
}

// Update shell state
void shell_update(void) {
    if (!shell_active) return;

    // Check for keyboard input
    if (keyboard_status()) {
        char c = keyboard_read_data();
        shell_input(c);
    }
}

// Check if shell is active
bool shell_is_active(void) {
    return shell_active;
}
