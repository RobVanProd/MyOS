#include "process.h"
#include "terminal.h"

// Test function that will run as a separate process
void test_process_function(void) {
    int pid = sys_getpid();
    terminal_writestring("Test process ");
    terminal_writedec(pid);
    terminal_writestring(" running\n");
    
    // Do some work
    for(int i = 0; i < 1000000; i++) {
        asm volatile("nop");
    }
    
    terminal_writestring("Test process ");
    terminal_writedec(pid);
    terminal_writestring(" exiting\n");
    sys_exit(0);
}

void test_process_management(void) {
    terminal_writestring("Starting process management tests...\n");
    
    // Test 1: Create a simple process
    terminal_writestring("Test 1: Creating simple process\n");
    process_t* proc = process_create("test1", test_process_function);
    if (!proc) {
        terminal_writestring("Failed to create process\n");
        return;
    }
    terminal_writestring("Created process with PID ");
    terminal_writedec(proc->pid);
    terminal_writestring("\n");
    
    // Test 2: Fork test
    terminal_writestring("Test 2: Testing fork\n");
    int child_pid = sys_fork();
    if (child_pid < 0) {
        terminal_writestring("Fork failed\n");
    } else if (child_pid == 0) {
        // Child process
        terminal_writestring("Child process running (PID: ");
        terminal_writedec(sys_getpid());
        terminal_writestring(")\n");
        sys_exit(0);
    } else {
        // Parent process
        terminal_writestring("Parent process (PID: ");
        terminal_writedec(sys_getpid());
        terminal_writestring(") created child with PID ");
        terminal_writedec(child_pid);
        terminal_writestring("\n");
        
        // Wait for child
        int status;
        int wait_pid = sys_wait(&status);
        terminal_writestring("Child process ");
        terminal_writedec(wait_pid);
        terminal_writestring(" exited\n");
    }
    
    // Test 3: Multiple processes
    terminal_writestring("Test 3: Creating multiple processes\n");
    for (int i = 0; i < 3; i++) {
        process_t* p = process_create("test_multi", test_process_function);
        if (p) {
            terminal_writestring("Created process with PID ");
            terminal_writedec(p->pid);
            terminal_writestring("\n");
        }
    }
    
    // Test 4: Process kill
    terminal_writestring("Test 4: Testing process kill\n");
    process_t* kill_proc = process_create("test_kill", test_process_function);
    if (kill_proc) {
        terminal_writestring("Created process with PID ");
        terminal_writedec(kill_proc->pid);
        terminal_writestring(" for kill test\n");
        
        // Let it run for a bit
        for(int i = 0; i < 100000; i++) {
            asm volatile("nop");
        }
        
        // Kill it
        if (sys_kill(kill_proc->pid, 9) == 0) {
            terminal_writestring("Successfully killed process ");
            terminal_writedec(kill_proc->pid);
            terminal_writestring("\n");
        } else {
            terminal_writestring("Failed to kill process\n");
        }
    }
    
    terminal_writestring("Process management tests completed\n");
}
