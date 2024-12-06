#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

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

// Command system functions
void command_init(void);
int command_execute(const char* cmd);
int command_register(const char* name, const char* description, command_func_t func);

#endif /* COMMAND_H */
