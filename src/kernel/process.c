#include "process.h"
#include "memory.h"
#include "terminal.h"

// Global variables
process_t* current_process = NULL;
static uint32_t next_pid = 1;
static process_t* processes[MAX_PROCESSES] = {NULL};

// Initialize process management
void process_init(void) {
    // Create kernel process
    process_t* kernel_process = kmalloc(sizeof(process_t));
    memset(kernel_process, 0, sizeof(process_t));

    kernel_process->pid = 0;  // Kernel is PID 0
    kernel_process->parent = NULL;
    kernel_process->state = PROCESS_STATE_RUNNING;
    kernel_process->priority = PROCESS_PRIORITY_HIGH;
    kernel_process->flags = PROCESS_FLAG_KERNEL;
    kernel_process->stack_size = 8192;  // 8KB stack for kernel
    kernel_process->stack = (uint32_t)kmalloc(kernel_process->stack_size);

    // Set up kernel page directory
    kernel_process->page_directory = get_kernel_page_directory();

    // Set up kernel stack
    kernel_process->stack_base = 0xC0000000 - kernel_process->stack_size;

    // Initialize kernel context
    memset(&kernel_process->context, 0, sizeof(process_context_t));
    kernel_process->context.esp = kernel_process->stack_base + kernel_process->stack_size;
    kernel_process->context.eflags = 0x202;  // Interrupts enabled

    // Set as current process
    current_process = kernel_process;
    processes[0] = kernel_process;

    // Initialize scheduler
    scheduler_init();
}

// Create a new process
process_t* process_create(const char* name, void (*entry)(void)) {
    // Allocate process structure
    process_t* process = kmalloc(sizeof(process_t));
    if (!process) return NULL;

    // Initialize process structure
    memset(process, 0, sizeof(process_t));
    process->pid = next_pid++;
    process->parent = current_process;
    strncpy(process->name, name, MAX_PROCESS_NAME - 1);
    process->state = PROCESS_STATE_READY;
    process->priority = PROCESS_PRIORITY_NORMAL;
    process->flags = PROCESS_FLAG_USER;
    process->stack_size = 4096;  // 4KB stack for user processes

    // Allocate stack
    process->stack = (uint32_t)kmalloc(process->stack_size);
    if (!process->stack) {
        kfree(process);
        return NULL;
    }

    // Create page directory
    process->page_directory = create_page_directory();
    if (!process->page_directory) {
        kfree((void*)process->stack);
        kfree(process);
        return NULL;
    }

    // Set up stack
    process->stack_base = 0xC0000000 - process->stack_size;
    if (!allocate_region(process->page_directory, process->stack_base, process->stack_size, 
                        PAGE_PRESENT | PAGE_WRITE | (process->flags & PROCESS_FLAG_USER ? PAGE_USER : 0))) {
        free_page_directory(process->page_directory);
        kfree((void*)process->stack);
        kfree(process);
        return NULL;
    }

    // Initialize context
    memset(&process->context, 0, sizeof(process_context_t));
    process->context.esp = process->stack_base + process->stack_size;
    process->context.eip = (uint32_t)entry;
    process->context.eflags = 0x202;  // Interrupts enabled

    // Add to process list
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i]) {
            processes[i] = process;
            break;
        }
    }

    // Add to scheduler
    scheduler_add_process(process);

    return process;
}

// Destroy a process
void process_destroy(process_t* process) {
    if (!process) return;

    // Remove from scheduler
    scheduler_remove_process(process);

    // Remove from process list
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == process) {
            processes[i] = NULL;
            break;
        }
    }

    // Free resources
    if (process->stack) {
        kfree((void*)process->stack);
    }

    if (process->page_directory) {
        free_page_directory(process->page_directory);
    }

    kfree(process);
}

// Switch to a process
void process_switch(process_t* next) {
    if (!next || next == current_process) return;

    process_t* prev = current_process;
    
    // Save current context if there is a previous process
    if (prev && prev->state != PROCESS_STATE_ZOMBIE) {
        __asm__ volatile(
            "mov %%eax, %0\n\t"
            "mov %%ebx, %1\n\t"
            "mov %%ecx, %2\n\t"
            "mov %%edx, %3\n\t"
            "mov %%esi, %4\n\t"
            "mov %%edi, %5\n\t"
            "mov %%ebp, %6\n\t"
            : "=m"(prev->context.eax),
              "=m"(prev->context.ebx),
              "=m"(prev->context.ecx),
              "=m"(prev->context.edx),
              "=m"(prev->context.esi),
              "=m"(prev->context.edi),
              "=m"(prev->context.ebp)
            : : "memory"
        );

        __asm__ volatile(
            "pushf\n\t"
            "pop %%eax\n\t"
            "mov %%eax, %0\n\t"
            : "=m"(prev->context.eflags)
            : : "eax"
        );

        prev->context.esp = (uint32_t)&next;
        prev->context.eip = (uint32_t)&&return_label;  // Save return address
    }

    // Update process states
    if (prev) prev->state = PROCESS_STATE_READY;
    next->state = PROCESS_STATE_RUNNING;
    current_process = next;

    // Switch page directory if different
    if (!prev || prev->page_directory != next->page_directory) {
        switch_page_directory(next->page_directory);
    }

    // Load next context
    __asm__ volatile(
        "mov %0, %%eax\n\t"
        "mov %1, %%ebx\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "mov %4, %%esi\n\t"
        "mov %5, %%edi\n\t"
        "mov %6, %%ebp\n\t"
        "mov %7, %%esp\n\t"
        : : "m"(next->context.eax),
            "m"(next->context.ebx),
            "m"(next->context.ecx),
            "m"(next->context.edx),
            "m"(next->context.esi),
            "m"(next->context.edi),
            "m"(next->context.ebp),
            "m"(next->context.esp)
        : "memory"
    );

    __asm__ volatile(
        "push %0\n\t"
        "popf\n\t"
        : : "m"(next->context.eflags)
    );

    // Jump to next process's instruction pointer
    __asm__ volatile(
        "jmp *%0\n\t"
        : : "m"(next->context.eip)
    );

return_label:
    return;
}

// Yield to next process
void process_yield(void) {
    process_t* next = scheduler_next_process();
    if (next && next != current_process) {
        process_switch(next);
    }
}

// Get process by PID
process_t* process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] && processes[i]->pid == pid) {
            return processes[i];
        }
    }
    return NULL;
}

// Initialize scheduler
void scheduler_init(void) {
    // Initialize scheduler data structures
    memset(processes, 0, sizeof(processes));
    current_process = NULL;
}

// Add process to scheduler
void scheduler_add_process(process_t* process) {
    if (!process) return;

    // Find empty slot in process array
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i]) {
            processes[i] = process;
            process->state = PROCESS_STATE_READY;
            process->next = NULL;
            return;
        }
    }

    // No empty slots found
    terminal_writestring("Error: Maximum number of processes reached\n");
    process_destroy(process);
}

// Remove process from scheduler
void scheduler_remove_process(process_t* process) {
    if (!process) return;

    // Find and remove process from array
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == process) {
            processes[i] = NULL;
            process->state = PROCESS_STATE_ZOMBIE;
            process->next = NULL;

            // If removing current process, schedule next one
            if (process == current_process) {
                process_t* next = scheduler_next_process();
                if (next && next != process) {
                    process_switch(next);
                }
            }
            return;
        }
    }
}

// Get next process to run using round-robin scheduling
process_t* scheduler_next_process(void) {
    static int last_scheduled = -1;
    
    // Start from the next process after the last scheduled one
    int start = (last_scheduled + 1) % MAX_PROCESSES;
    int i = start;

    do {
        if (processes[i] && 
            processes[i]->state == PROCESS_STATE_READY && 
            processes[i] != current_process) {
            last_scheduled = i;
            return processes[i];
        }
        i = (i + 1) % MAX_PROCESSES;
    } while (i != start);

    // If no other ready process found, continue with current process
    return current_process;
}

// System call implementations
int sys_fork(void) {
    process_t* parent = current_process;
    if (!parent) return -1;

    // Check if we've reached the process limit
    int free_slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i]) {
            free_slot = i;
            break;
        }
    }
    if (free_slot == -1) return -1;

    // Allocate new process structure
    process_t* child = kmalloc(sizeof(process_t));
    if (!child) return -1;

    // Copy process structure
    memcpy(child, parent, sizeof(process_t));
    child->pid = next_pid++;
    child->parent = parent;
    child->state = PROCESS_STATE_READY;
    child->next = NULL;

    // Copy page directory
    child->page_directory = copy_page_directory(parent->page_directory);
    if (!child->page_directory) {
        kfree(child);
        return -1;
    }

    // Allocate and copy stack
    child->stack = (uint32_t)kmalloc(parent->stack_size);
    if (!child->stack) {
        free_page_directory(child->page_directory);
        kfree(child);
        return -1;
    }
    memcpy((void*)child->stack, (void*)parent->stack, parent->stack_size);

    // Update stack pointers to point to new stack
    uint32_t stack_offset = child->stack - parent->stack;
    child->context.esp += stack_offset;
    child->stack_base = parent->stack_base + stack_offset;

    // Set return value for child
    child->context.eax = 0;  // Child gets 0

    // Add to process list and scheduler
    processes[free_slot] = child;
    scheduler_add_process(child);

    return child->pid;  // Parent gets child's pid
}

int sys_exec(const char* path, char* const argv[]) {
    if (!path || !current_process) return -1;

    // TODO: Load program from filesystem
    // For now, just return error
    return -1;
}

void sys_exit(int status) {
    process_t* current = current_process;
    if (!current) return;

    // Mark process as zombie and store exit status
    current->state = PROCESS_STATE_ZOMBIE;
    current->context.eax = status;  // Store exit status in eax

    // Wake up parent if it's waiting
    process_t* parent = current->parent;
    if (parent && parent->state == PROCESS_STATE_BLOCKED) {
        parent->state = PROCESS_STATE_READY;
        process_wake(parent);
    }

    // If no parent, or parent already dead, cleanup immediately
    if (!parent || parent->state == PROCESS_STATE_ZOMBIE) {
        process_destroy(current);
    }

    // Switch to next process
    process_yield();  // Never returns
}

int sys_wait(int* status) {
    process_t* current = current_process;
    if (!current) return -1;

    // Check for zombie children
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t* proc = processes[i];
        if (proc && proc->parent == current && proc->state == PROCESS_STATE_ZOMBIE) {
            int pid = proc->pid;
            if (status) {
                *status = proc->context.eax;  // Get exit status from eax
            }
            process_destroy(proc);
            return pid;
        }
    }

    // Check if we have any children at all
    bool has_children = false;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t* proc = processes[i];
        if (proc && proc->parent == current && proc->state != PROCESS_STATE_ZOMBIE) {
            has_children = true;
            break;
        }
    }
    if (!has_children) return -1;

    // Block until a child exits
    current->state = PROCESS_STATE_BLOCKED;
    process_yield();
    return 0;  // Will be updated when we wake up
}

int sys_getpid(void) {
    return current_process ? current_process->pid : -1;
}

int sys_kill(int pid, int sig) {
    if (pid <= 0) return -1;  // Invalid pid

    process_t* proc = process_get_by_pid(pid);
    if (!proc) return -1;  // Process not found

    // Cannot kill kernel process (pid 0)
    if (proc->flags & PROCESS_FLAG_KERNEL) return -1;

    switch (sig) {
        case 9:  // SIGKILL
            proc->state = PROCESS_STATE_ZOMBIE;
            proc->context.eax = 128 + sig;  // Exit status for signal
            if (proc->parent && proc->parent->state == PROCESS_STATE_BLOCKED) {
                proc->parent->state = PROCESS_STATE_READY;
                process_wake(proc->parent);
            }
            if (proc == current_process) {
                process_yield();  // Never returns
            } else {
                process_destroy(proc);
            }
            break;
        default:
            return -1;  // Unsupported signal
    }

    return 0;
}