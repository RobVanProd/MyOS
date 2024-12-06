# Command System API

The Command System API provides interfaces for registering and executing commands in MyOS. This documentation covers both the kernel-level command system and the user-level shell interface.

## Overview

The command system consists of two main components:
1. Kernel Command System (`command.h`)
2. Shell Interface (`shell.h`)

## Kernel Command System

### Header File
```c
#include <kernel/command.h>
```

### Data Structures

#### Command Structure
```c
typedef struct {
    const char* name;        // Command name
    const char* description; // Command description
    command_func_t func;     // Command handler function
} command_t;
```

#### Command Function Type
```c
typedef int (*command_func_t)(int argc, char* argv[]);
```

### Constants

```c
#define MAX_COMMAND_LENGTH 256  // Maximum command line length
#define MAX_ARGS 16            // Maximum number of arguments
```

### Functions

#### Command System Initialization
```c
void command_init(void);
```
Initializes the command system. Must be called before using any command functions.

#### Command Registration
```c
int command_register(const char* name, const char* description, command_func_t func);
```
Registers a new command with the system.

Parameters:
- `name`: Command name (case-sensitive)
- `description`: Brief description of the command
- `func`: Function pointer to command handler

Returns:
- 0 on success
- -1 if maximum commands reached or invalid parameters

#### Command Execution
```c
int command_execute(const char* cmdline);
```
Executes a command line string.

Parameters:
- `cmdline`: Complete command line including arguments

Returns:
- Command's return value on success
- -1 on parsing error or command not found

## Shell Interface

### Header File
```c
#include <apps/shell.h>
```

### Data Structures

#### Shell Structure
```c
typedef struct {
    window_t* window;                    // Shell window
    char buffer[SHELL_BUFFER_SIZE];      // Input buffer
    int buffer_pos;                      // Current position in buffer
    char* history[SHELL_MAX_HISTORY];    // Command history
    int history_count;                   // Number of history entries
    int history_pos;                     // Current position in history
    int cursor_x;                        // Cursor X position
    int cursor_y;                        // Cursor Y position
    bool insert_mode;                    // Insert/overwrite mode
} shell_t;
```

### Constants

```c
#define SHELL_BUFFER_SIZE 4096   // Shell buffer size
#define SHELL_MAX_HISTORY 50     // Maximum history entries
#define SHELL_PROMPT "MyOS> "    // Default prompt
```

### Functions

#### Shell Creation
```c
shell_t* create_shell(int x, int y, int width, int height);
```
Creates a new shell window.

Parameters:
- `x`, `y`: Window position
- `width`, `height`: Window dimensions

Returns:
- Pointer to shell structure on success
- NULL on failure

#### Shell Destruction
```c
void destroy_shell(shell_t* shell);
```
Destroys a shell instance and frees resources.

#### Command Processing
```c
void shell_process_command(shell_t* shell, const char* command);
```
Processes a command in the shell context.

#### Output Functions
```c
void shell_print(shell_t* shell, const char* text);
void shell_println(shell_t* shell, const char* text);
```
Print text to the shell window.

#### Shell Control
```c
void shell_clear(shell_t* shell);
```
Clears the shell window.

### Event Handlers

```c
void shell_handle_key(window_t* window, int key);
void shell_draw(window_t* window);
```
Internal handlers for keyboard input and window drawing.

## Built-in Commands

### Implementation Guidelines

When implementing a command handler:

1. Check argument count
2. Validate arguments
3. Provide helpful usage messages
4. Return appropriate status codes

Example command implementation:
```c
int cmd_example(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <argument>\n", argv[0]);
        return -1;
    }
    
    // Command implementation
    return 0;
}
```

### Return Values

Command handlers should return:
- 0 for success
- -1 for usage/argument errors
- Other negative values for specific errors

## Error Handling

The command system provides several error handling mechanisms:

1. Command Registration Errors
   - Maximum commands exceeded
   - Invalid parameters
   - Duplicate commands

2. Command Execution Errors
   - Command not found
   - Argument parsing errors
   - Command-specific errors

3. Shell Errors
   - Window creation failures
   - Memory allocation errors
   - Input/output errors

## Examples

### Registering a Command
```c
int cmd_hello(int argc, char* argv[]) {
    printf("Hello, World!\n");
    return 0;
}

// In initialization code:
command_register("hello", "Displays a greeting", cmd_hello);
```

### Creating a Shell
```c
shell_t* shell = create_shell(100, 100, 640, 400);
if (shell) {
    // Shell created successfully
} else {
    // Handle error
}
```

## Best Practices

1. Command Names
   - Use lowercase
   - Keep names short but descriptive
   - Avoid special characters

2. Command Descriptions
   - Start with a verb
   - Be concise but clear
   - Include argument format

3. Error Handling
   - Always check return values
   - Provide clear error messages
   - Clean up resources on error

4. User Interface
   - Provide consistent feedback
   - Include help information
   - Support standard conventions

## See Also

- [Shell User Guide](../user/shell.md)
- [Window System API](window.md)
- [Process Management API](process.md)
- [File System API](filesystem.md)
