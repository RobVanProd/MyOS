#include "command.h"
#include "terminal.h"
#include "memory.h"
#include "fs.h"
#include <string.h>

#define MAX_COMMANDS 32

// Array of registered commands
static command_t commands[MAX_COMMANDS];
static int command_count = 0;

// Initialize command system
void command_init(void) {
    command_count = 0;
    
    // Register built-in commands
    command_register("make", "Compile and build programs", cmd_make);
    command_register("help", "Display available commands", cmd_help);
}

// Register a new command
int command_register(const char* name, const char* description, command_func_t func) {
    if (command_count >= MAX_COMMANDS) {
        return -1;
    }
    
    commands[command_count].name = name;
    commands[command_count].description = description;
    commands[command_count].func = func;
    command_count++;
    
    return 0;
}

// Parse command line into arguments
static int parse_args(char* cmdline, char* argv[]) {
    int argc = 0;
    char* token = strtok(cmdline, " ");
    
    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    return argc;
}

// Execute a command
int command_execute(const char* cmdline) {
    char buf[MAX_COMMAND_LENGTH];
    char* argv[MAX_ARGS];
    
    // Copy command line to buffer
    strncpy(buf, cmdline, MAX_COMMAND_LENGTH - 1);
    buf[MAX_COMMAND_LENGTH - 1] = '\0';
    
    // Parse arguments
    int argc = parse_args(buf, argv);
    if (argc == 0) {
        return -1;
    }
    
    // Find command
    for (int i = 0; i < command_count; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            return commands[i].func(argc, argv);
        }
    }
    
    terminal_writestring("Unknown command: ");
    terminal_writestring(argv[0]);
    terminal_writestring("\n");
    return -1;
}

// Built-in make command
int cmd_make(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_writestring("Usage: make <target>\n");
        return -1;
    }
    
    const char* target = argv[1];
    terminal_writestring("Building target: ");
    terminal_writestring(target);
    terminal_writestring("\n");
    
    // Check if Makefile exists
    int fd = fs_open("Makefile", 0);
    if (fd < 0) {
        terminal_writestring("Error: Makefile not found\n");
        return -1;
    }
    
    // TODO: Parse Makefile and execute build commands
    // For now, just simulate a build process
    terminal_writestring("Compiling...\n");
    for (int i = 0; i < 3; i++) {
        terminal_writestring(".");
        // Add a small delay
        for (volatile int j = 0; j < 1000000; j++);
    }
    terminal_writestring("\nBuild complete!\n");
    
    fs_close(fd);
    return 0;
}

// Built-in help command
int cmd_help(int argc, char* argv[]) {
    terminal_writestring("Available commands:\n");
    for (int i = 0; i < command_count; i++) {
        terminal_writestring("  ");
        terminal_writestring(commands[i].name);
        terminal_writestring(" - ");
        terminal_writestring(commands[i].description);
        terminal_writestring("\n");
    }
    return 0;
}
