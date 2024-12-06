#include "shell.h"
#include "../kernel/terminal.h"
#include "../kernel/memory.h"
#include "../kernel/fs.h"
#include "../kernel/time.h"
#include <string.h>
#include <stdio.h>

// Command structure
typedef struct {
    const char* name;
    const char* description;
    void (*func)(shell_t*, int, char**);
} shell_command_t;

// Built-in commands table
static const shell_command_t commands[] = {
    {"cd", "Change directory", shell_cmd_cd},
    {"dir", "List directory contents", shell_cmd_dir},
    {"echo", "Display messages", shell_cmd_echo},
    {"cls", "Clear screen", shell_cmd_cls},
    {"type", "Display file contents", shell_cmd_type},
    {"copy", "Copy files", shell_cmd_copy},
    {"del", "Delete files", shell_cmd_del},
    {"mkdir", "Create directory", shell_cmd_mkdir},
    {"rmdir", "Remove directory", shell_cmd_rmdir},
    {"date", "Show/set date", shell_cmd_date},
    {"time", "Show/set time", shell_cmd_time},
    {"ver", "Show OS version", shell_cmd_ver},
    {"help", "Show help", shell_cmd_help},
    {NULL, NULL, NULL}
};

// Create a new shell window
shell_t* create_shell(int x, int y, int width, int height) {
    shell_t* shell = heap_alloc(sizeof(shell_t));
    if (!shell) return NULL;

    // Initialize shell structure
    memset(shell, 0, sizeof(shell_t));
    shell->window = create_window(x, y, width, height, "MyOS Shell", 
                                WINDOW_MOVABLE | WINDOW_RESIZABLE | WINDOW_HAS_TITLE);
    if (!shell->window) {
        heap_free(shell);
        return NULL;
    }

    // Set window callbacks
    shell->window->data = shell;
    shell->window->on_key = shell_handle_key;
    shell->window->on_draw = shell_draw;

    // Initialize shell state
    shell->insert_mode = true;
    shell_clear(shell);
    shell_println(shell, "MyOS Shell [Version 1.0]");
    shell_println(shell, "Type 'help' for list of commands.");
    shell_print(shell, SHELL_PROMPT);

    return shell;
}

void destroy_shell(shell_t* shell) {
    if (!shell) return;
    destroy_window(shell->window);
    heap_free(shell);
}

// Parse command line into arguments
static int parse_args(char* cmdline, char* argv[]) {
    int argc = 0;
    char* token = strtok(cmdline, " \t");
    
    while (token && argc < 16) {  // Limit to 16 arguments
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    
    return argc;
}

// Process a command
void shell_process_command(shell_t* shell, const char* command) {
    char cmd_buf[256];
    char* argv[16];
    
    // Copy command to buffer for parsing
    strncpy(cmd_buf, command, sizeof(cmd_buf) - 1);
    cmd_buf[sizeof(cmd_buf) - 1] = '\0';
    
    // Parse arguments
    int argc = parse_args(cmd_buf, argv);
    if (argc == 0) {
        shell_print(shell, SHELL_PROMPT);
        return;
    }
    
    // Find and execute command
    for (const shell_command_t* cmd = commands; cmd->name; cmd++) {
        if (strcmp(argv[0], cmd->name) == 0) {
            cmd->func(shell, argc, argv);
            shell_print(shell, SHELL_PROMPT);
            return;
        }
    }
    
    // Command not found
    shell_println(shell, "Unknown command. Type 'help' for list of commands.");
    shell_print(shell, SHELL_PROMPT);
}

// Clear the shell window
void shell_clear(shell_t* shell) {
    shell->cursor_x = 0;
    shell->cursor_y = 0;
    shell->buffer_pos = 0;
    shell->buffer[0] = '\0';
    window_clear(shell->window);
}

// Print text to shell
void shell_print(shell_t* shell, const char* text) {
    while (*text) {
        if (*text == '\n') {
            shell->cursor_x = 0;
            shell->cursor_y++;
        } else {
            window_putchar(shell->window, shell->cursor_x * 8, shell->cursor_y * 16, *text);
            shell->cursor_x++;
        }
        
        if (shell->cursor_x >= shell->window->width / 8) {
            shell->cursor_x = 0;
            shell->cursor_y++;
        }
        
        text++;
    }
    window_invalidate(shell->window);
}

// Print text with newline
void shell_println(shell_t* shell, const char* text) {
    shell_print(shell, text);
    shell_print(shell, "\n");
}

// Handle keyboard input
void shell_handle_key(window_t* window, int key) {
    shell_t* shell = (shell_t*)window->data;
    
    if (key == '\n') {
        shell_println(shell, "");
        shell->buffer[shell->buffer_pos] = '\0';
        
        // Add to history if non-empty
        if (shell->buffer_pos > 0) {
            if (shell->history_count < SHELL_MAX_HISTORY) {
                shell->history[shell->history_count] = strdup(shell->buffer);
                shell->history_count++;
            }
        }
        
        shell_process_command(shell, shell->buffer);
        shell->buffer_pos = 0;
        shell->history_pos = shell->history_count;
        
    } else if (key == '\b') {
        if (shell->buffer_pos > 0) {
            shell->buffer_pos--;
            shell->cursor_x--;
            window_putchar(window, shell->cursor_x * 8, shell->cursor_y * 16, ' ');
            window_invalidate(window);
        }
        
    } else if (key >= ' ' && key <= '~' && shell->buffer_pos < SHELL_BUFFER_SIZE - 1) {
        shell->buffer[shell->buffer_pos++] = key;
        window_putchar(window, shell->cursor_x * 8, shell->cursor_y * 16, key);
        shell->cursor_x++;
        window_invalidate(window);
    }
}

// Draw shell window
void shell_draw(window_t* window) {
    shell_t* shell = (shell_t*)window->data;
    
    // Draw cursor
    if ((get_timer_ticks() / 10) % 2 == 0) {
        window_putchar(window, shell->cursor_x * 8, shell->cursor_y * 16, '_');
    }
}

// Built-in command implementations
void shell_cmd_help(shell_t* shell, int argc, char* argv[]) {
    shell_println(shell, "Available commands:");
    for (const shell_command_t* cmd = commands; cmd->name; cmd++) {
        char buf[80];
        snprintf(buf, sizeof(buf), "  %-10s - %s", cmd->name, cmd->description);
        shell_println(shell, buf);
    }
}

void shell_cmd_cls(shell_t* shell, int argc, char* argv[]) {
    shell_clear(shell);
}

void shell_cmd_echo(shell_t* shell, int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        shell_print(shell, argv[i]);
        if (i < argc - 1) shell_print(shell, " ");
    }
    shell_println(shell, "");
}

void shell_cmd_ver(shell_t* shell, int argc, char* argv[]) {
    shell_println(shell, "MyOS [Version 1.0]");
    shell_println(shell, "Copyright (c) 2024 MyOS Development Team");
}

// Directory commands
void shell_cmd_dir(shell_t* shell, int argc, char* argv[]) {
    // TODO: Implement directory listing
    shell_println(shell, "Directory listing not implemented yet");
}

void shell_cmd_cd(shell_t* shell, int argc, char* argv[]) {
    if (argc < 2) {
        shell_println(shell, "Current directory: /");
        return;
    }
    // TODO: Implement directory change
    shell_println(shell, "Directory change not implemented yet");
}

void shell_cmd_mkdir(shell_t* shell, int argc, char* argv[]) {
    if (argc < 2) {
        shell_println(shell, "Usage: mkdir <directory>");
        return;
    }
    // TODO: Implement directory creation
    shell_println(shell, "Directory creation not implemented yet");
}

void shell_cmd_rmdir(shell_t* shell, int argc, char* argv[]) {
    if (argc < 2) {
        shell_println(shell, "Usage: rmdir <directory>");
        return;
    }
    // TODO: Implement directory removal
    shell_println(shell, "Directory removal not implemented yet");
}

// File commands
void shell_cmd_type(shell_t* shell, int argc, char* argv[]) {
    if (argc < 2) {
        shell_println(shell, "Usage: type <file>");
        return;
    }
    // TODO: Implement file viewing
    shell_println(shell, "File viewing not implemented yet");
}

void shell_cmd_copy(shell_t* shell, int argc, char* argv[]) {
    if (argc < 3) {
        shell_println(shell, "Usage: copy <source> <destination>");
        return;
    }
    // TODO: Implement file copying
    shell_println(shell, "File copying not implemented yet");
}

void shell_cmd_del(shell_t* shell, int argc, char* argv[]) {
    if (argc < 2) {
        shell_println(shell, "Usage: del <file>");
        return;
    }
    // TODO: Implement file deletion
    shell_println(shell, "File deletion not implemented yet");
}

// Time and date commands
void shell_cmd_time(shell_t* shell, int argc, char* argv[]) {
    char buf[32];
    time_t time = get_system_time();
    format_time(buf, sizeof(buf), time);
    shell_println(shell, buf);
}

void shell_cmd_date(shell_t* shell, int argc, char* argv[]) {
    char buf[32];
    time_t time = get_system_time();
    format_date(buf, sizeof(buf), time);
    shell_println(shell, buf);
}
