#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stdint.h>
#include <stdbool.h>

// Signal types
#define SIGKILL     9   // Kill signal
#define SIGTERM    15   // Termination request
#define SIGINT      2   // Interrupt signal
#define SIGSTOP    19   // Stop process
#define SIGCONT    18   // Continue process if stopped

// Signal handler type
typedef void (*signal_handler_t)(int);

// Signal handling functions
void signal_init(void);
void register_signal_handler(int signum, signal_handler_t handler);
int send_signal(uint32_t pid, int signum);
void check_pending_signals(void);
bool has_pending_signals(void);
void default_signal_handler(int signum);

#endif /* _SIGNAL_H */
