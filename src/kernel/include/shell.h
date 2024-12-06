#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>

// Shell functions
void shell_init(void);
void create_shell(void);
void shell_input(char c);
void shell_update(void);
bool shell_is_active(void);

#endif /* SHELL_H */
