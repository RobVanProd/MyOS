#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <stdbool.h>

// Maximum command length
#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 16

// Command function type
typedef int (*command_func_t)(int argc, char* argv[]);

// Command structure
typedef struct {
    const char* name;
    const char* description;
    command_func_t func;
} command_t;

// Initialize command system
void command_init(void);

// Register a new command
int command_register(const char* name, const char* description, command_func_t func);

// Execute a command
int command_execute(const char* cmdline);

// Built-in commands
int cmd_make(int argc, char* argv[]);
int cmd_help(int argc, char* argv[]);

#endif // COMMAND_H
