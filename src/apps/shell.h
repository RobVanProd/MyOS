#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stdbool.h>
#include "window.h"

#define SHELL_BUFFER_SIZE 4096
#define SHELL_MAX_HISTORY 50
#define SHELL_PROMPT "MyOS> "

typedef struct {
    window_t* window;
    char buffer[SHELL_BUFFER_SIZE];
    int buffer_pos;
    char* history[SHELL_MAX_HISTORY];
    int history_count;
    int history_pos;
    int cursor_x;
    int cursor_y;
    bool insert_mode;
} shell_t;

// Shell functions
shell_t* create_shell(int x, int y, int width, int height);
void destroy_shell(shell_t* shell);
void shell_process_command(shell_t* shell, const char* command);
void shell_clear(shell_t* shell);
void shell_print(shell_t* shell, const char* text);
void shell_println(shell_t* shell, const char* text);
void shell_handle_key(window_t* window, int key);
void shell_draw(window_t* window);

// Built-in shell commands
void shell_cmd_cd(shell_t* shell, int argc, char* argv[]);
void shell_cmd_dir(shell_t* shell, int argc, char* argv[]);
void shell_cmd_echo(shell_t* shell, int argc, char* argv[]);
void shell_cmd_cls(shell_t* shell, int argc, char* argv[]);
void shell_cmd_type(shell_t* shell, int argc, char* argv[]);
void shell_cmd_copy(shell_t* shell, int argc, char* argv[]);
void shell_cmd_del(shell_t* shell, int argc, char* argv[]);
void shell_cmd_mkdir(shell_t* shell, int argc, char* argv[]);
void shell_cmd_rmdir(shell_t* shell, int argc, char* argv[]);
void shell_cmd_date(shell_t* shell, int argc, char* argv[]);
void shell_cmd_time(shell_t* shell, int argc, char* argv[]);
void shell_cmd_ver(shell_t* shell, int argc, char* argv[]);
void shell_cmd_help(shell_t* shell, int argc, char* argv[]);

#endif // SHELL_H
