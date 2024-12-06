#include "command.h"
#include "terminal.h"
#include "string.h"
#include "process.h"

#define MAX_COMMANDS 32
#define MAX_ARGS 16

static struct command {
    const char* name;
    const char* description;
    int (*func)(int argc, char* argv[]);
} commands[MAX_COMMANDS];

static int num_commands = 0;

// Forward declarations
int cmd_help(int argc, char* argv[]);
int cmd_make(int argc, char* argv[]);

// Initialize command system
void command_init(void) {
    command_register("make", "Compile and build programs", cmd_make);
    command_register("help", "Display available commands", cmd_help);
}

// Register a new command
int command_register(const char* name, const char* description, int (*func)(int argc, char* argv[])) {
    if (num_commands >= MAX_COMMANDS) {
        return -1;
    }

    commands[num_commands].name = name;
    commands[num_commands].description = description;
    commands[num_commands].func = func;
    num_commands++;

    return 0;
}

// Execute a command
int command_execute(const char* cmdline) {
    char* argv[MAX_ARGS];
    int argc = 0;
    char cmdline_copy[256];
    
    // Make a copy of cmdline since strtok modifies the string
    strncpy(cmdline_copy, cmdline, sizeof(cmdline_copy) - 1);
    cmdline_copy[sizeof(cmdline_copy) - 1] = '\0';
    
    // Parse command line into arguments
    char* token = strtok(cmdline_copy, " ");
    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    if (argc == 0) {
        return 0;  // Empty command line
    }
    
    // Find and execute command
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            return commands[i].func(argc, argv);
        }
    }
    
    terminal_writestring("Unknown command: ");
    terminal_writestring(argv[0]);
    terminal_writestring("\n");
    return -1;
}

// Built-in commands
int cmd_help(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    terminal_writestring("Available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        terminal_writestring("  ");
        terminal_writestring(commands[i].name);
        terminal_writestring(" - ");
        terminal_writestring(commands[i].description);
        terminal_writestring("\n");
    }
    return 0;
}

int cmd_make(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_writestring("Usage: make <target>\n");
        return -1;
    }
    
    // TODO: Implement build system
    terminal_writestring("Building target: ");
    terminal_writestring(argv[1]);
    terminal_writestring("\n");
    return 0;
}
