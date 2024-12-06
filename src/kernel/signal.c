#include "signal.h"
#include "process.h"
#include "terminal.h"

#define MAX_SIGNALS 32

// Signal handler table
static signal_handler_t signal_handlers[MAX_SIGNALS] = {0};
static uint32_t pending_signals = 0;

// Initialize signal handling
void signal_init(void) {
    // Set all handlers to default
    for (int i = 0; i < MAX_SIGNALS; i++) {
        signal_handlers[i] = default_signal_handler;
    }
    pending_signals = 0;
}

// Register a signal handler
void register_signal_handler(int signum, signal_handler_t handler) {
    if (signum < 0 || signum >= MAX_SIGNALS) {
        return;
    }
    signal_handlers[signum] = handler ? handler : default_signal_handler;
}

// Send a signal to a process
int send_signal(uint32_t pid, int signum) {
    if (signum < 0 || signum >= MAX_SIGNALS) {
        return -1;
    }
    
    // For now, just set the pending signal bit
    pending_signals |= (1 << signum);
    return 0;
}

// Check and handle pending signals
void check_pending_signals(void) {
    // Process each pending signal
    for (int signum = 0; signum < MAX_SIGNALS; signum++) {
        if (pending_signals & (1 << signum)) {
            // Clear the pending bit
            pending_signals &= ~(1 << signum);
            
            // Call the handler
            if (signal_handlers[signum]) {
                signal_handlers[signum](signum);
            }
        }
    }
}

// Check if there are any pending signals
bool has_pending_signals(void) {
    return pending_signals != 0;
}

// Default signal handler
void default_signal_handler(int signum) {
    switch (signum) {
        case SIGKILL:
        case SIGTERM:
            terminal_writestring("\nProcess terminated\n");
            // TODO: Implement process termination
            break;
            
        case SIGINT:
            terminal_writestring("\nProcess interrupted\n");
            // TODO: Implement process interruption
            break;
            
        case SIGSTOP:
            terminal_writestring("\nProcess stopped\n");
            // TODO: Implement process stopping
            break;
            
        case SIGCONT:
            terminal_writestring("\nProcess continued\n");
            // TODO: Implement process continuation
            break;
            
        default:
            terminal_writestring("\nUnhandled signal\n");
            break;
    }
}
