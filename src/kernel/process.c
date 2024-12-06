#include "process.h"
#include "memory.h"
#include "terminal.h"

// Global variables
static process_t* current_process = NULL;
static process_t* process_list = NULL;
static uint32_t next_pid = 1;
static process_t* processes[MAX_PROCESSES] = {NULL};

// Initialize process management
void process_init(void) {
    // Create kernel process (PID 0)
    process_t* kernel_process = kmalloc(sizeof(process_t));
    kernel_process->pid = 0;
    kernel_process->parent_pid = 0;
    strncpy(kernel_process->name, "kernel", sizeof(kernel_process->name) - 1);
    kernel_process->name[sizeof(kernel_process->name) - 1] = '\0';
    kernel_process->state = PROCESS_STATE_RUNNING;
    kernel_process->priority = PRIORITY_HIGH;
    kernel_process->flags = PROCESS_FLAG_KERNEL;
    
    // Initialize context
    memset(&kernel_process->context, 0, sizeof(process_context_t));
    kernel_process->context.eip = (uint32_t)0xC0000000;  // Entry point
    kernel_process->context.eflags = 0x202;  // IF=1, bit 1 is reserved
    
    // Set up memory
    kernel_process->page_directory = get_kernel_page_directory();
    kernel_process->stack_size = 0x4000;  // 16KB stack
    kernel_process->stack_base = 0xC0000000 - kernel_process->stack_size;
    
    // Set up stack
    kernel_process->context.esp = kernel_process->stack_base + kernel_process->stack_size;
    kernel_process->context.ebp = kernel_process->context.esp;
    
    // Initialize process list
    current_process = kernel_process;
    process_list = kernel_process;
    processes[0] = kernel_process;
    
    // Initialize scheduler
    scheduler_init();
}

// Create a new process
process_t* process_create(const char* name, void* entry_point, uint8_t priority, uint8_t flags) {
    if (next_pid >= MAX_PROCESSES) return NULL;
    
    // Allocate process structure
    process_t* process = kmalloc(sizeof(process_t));
    if (!process) return NULL;
    
    // Initialize process
    process->pid = next_pid++;
    process->parent_pid = current_process ? current_process->pid : 0;
    strncpy(process->name, name, sizeof(process->name) - 1);
    process->name[sizeof(process->name) - 1] = '\0';
    process->state = PROCESS_STATE_READY;
    process->priority = priority;
    process->flags = flags;
    
    // Initialize context
    memset(&process->context, 0, sizeof(process_context_t));
    process->context.eip = (uint32_t)entry_point;
    process->context.eflags = 0x202;  // IF=1, bit 1 is reserved
    
    // Set up memory
    process->page_directory = create_page_directory();
    if (!process->page_directory) {
        kfree(process);
        return NULL;
    }
    
    // Allocate stack
    process->stack_size = 0x4000;  // 16KB stack
    process->stack_base = 0xC0000000 - process->stack_size;
    if (!allocate_region(process->page_directory, process->stack_base, process->stack_size, PAGE_PRESENT | PAGE_WRITE | (flags & PROCESS_FLAG_USER ? PAGE_USER : 0))) {
        free_page_directory(process->page_directory);
        kfree(process);
        return NULL;
    }
    
    // Set up stack
    process->context.esp = process->stack_base + process->stack_size;
    process->context.ebp = process->context.esp;
    
    // Add to process list
    process->next = process_list;
    process_list = process;
    processes[process->pid] = process;
    
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
    if (process == process_list) {
        process_list = process->next;
    } else {
        process_t* prev = process_list;
        while (prev && prev->next != process) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = process->next;
        }
    }
    
    // Free process resources
    if (process->page_directory) {
        free_page_directory(process->page_directory);
    }
    
    // Clear process slot
    processes[process->pid] = NULL;
    
    // Free process structure
    kfree(process);
}

// Switch to another process
void process_switch(process_t* next) {
    if (!next || next == current_process) return;
    
    process_t* prev = current_process;
    current_process = next;
    
    // Save current context
    asm volatile(
        "mov %%esp, %0\n"
        "mov %%ebp, %1\n"
        : "=r"(prev->context.esp), "=r"(prev->context.ebp)
    );
    
    // Load new context
    asm volatile(
        "mov %0, %%cr3\n"     // Switch page directory
        "mov %1, %%esp\n"     // Restore stack pointer
        "mov %2, %%ebp\n"     // Restore base pointer
        "push %3\n"           // Push EFLAGS
        "push %4\n"           // Push CS
        "push %5\n"           // Push EIP
        "iret\n"              // Return to new process
        :
        : "r"(next->context.cr3),
          "r"(next->context.esp),
          "r"(next->context.ebp),
          "r"(next->context.eflags),
          "r"(0x08),  // Kernel code segment
          "r"(next->context.eip)
    );
}

// Yield CPU to another process
void process_yield(void) {
    if (!current_process) return;
    process_t* next = scheduler_next_process();
    if (next) {
        process_switch(next);
    }
}

// Get current process
process_t* process_get_current(void) {
    return current_process;
}

// Get process by PID
process_t* process_get_by_pid(uint32_t pid) {
    if (pid >= MAX_PROCESSES) return NULL;
    return processes[pid];
}

// Initialize scheduler
void scheduler_init(void) {
    // Nothing to do yet
}

// Add process to scheduler
void scheduler_add_process(process_t* process) {
    if (!process) return;
    
    // Simple round-robin scheduling
    process->state = PROCESS_STATE_READY;
    process->next = process_list;
    process_list = process;
}

// Remove process from scheduler
void scheduler_remove_process(process_t* process) {
    if (!process) return;
    
    process->state = PROCESS_STATE_ZOMBIE;
    
    // Remove from process list
    if (process_list == process) {
        process_list = process->next;
    } else {
        process_t* prev = process_list;
        while (prev && prev->next != process) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = process->next;
        }
    }
}

// Scheduler tick handler
void scheduler_tick(void) {
    if (!current_process) return;
    process_yield();
}

// Get next process to run
process_t* scheduler_next_process(void) {
    if (!process_list) return NULL;
    
    // Simple round-robin scheduling
    process_t* next = current_process ? current_process->next : process_list;
    if (!next) next = process_list;
    
    while (next != current_process) {
        if (next->state == PROCESS_STATE_READY) {
            return next;
        }
        next = next->next ? next->next : process_list;
    }
    
    return NULL;
}

// System call implementations
int sys_fork(void) {
    process_t* parent = process_get_current();
    if (!parent) return -1;
    
    // Create new process structure
    process_t* child = kmalloc(sizeof(process_t));
    if (!child) return -1;
    
    // Copy process attributes
    memcpy(child, parent, sizeof(process_t));
    child->pid = next_pid++;
    child->parent_pid = parent->pid;
    child->state = PROCESS_STATE_READY;
    
    // Create new page directory and copy parent's memory
    child->page_directory = copy_page_directory(parent->page_directory);
    if (!child->page_directory) {
        kfree(child);
        return -1;
    }
    
    // Set up new stack
    child->stack_base = parent->stack_base;
    child->stack_size = parent->stack_size;
    
    // Copy parent's context but adjust return value
    memcpy(&child->context, &parent->context, sizeof(process_context_t));
    child->context.eax = 0; // Child gets 0 as return value
    
    // Add to process list and scheduler
    child->next = process_list;
    process_list = child;
    processes[child->pid] = child;
    scheduler_add_process(child);
    
    return child->pid;
}

int sys_exec(const char* path, char* const argv[]) {
    // TODO: Implement loading and executing a new program
    // For now, return error
    return -1;
}

void sys_exit(int status) {
    process_t* current = process_get_current();
    if (!current) return;
    
    // Set process state to zombie and store exit status
    current->state = PROCESS_STATE_ZOMBIE;
    
    // Wake up parent if it's waiting
    process_t* parent = process_get_by_pid(current->parent_pid);
    if (parent && parent->state == PROCESS_STATE_BLOCKED) {
        parent->state = PROCESS_STATE_READY;
    }
    
    // Switch to next process
    process_yield();
}

int sys_wait(int* status) {
    process_t* current = process_get_current();
    if (!current) return -1;
    
    // Check if any children are zombies
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        process_t* proc = processes[i];
        if (proc && proc->parent_pid == current->pid && proc->state == PROCESS_STATE_ZOMBIE) {
            // Get exit status
            if (status) {
                *status = 0; // TODO: Store actual exit status
            }
            
            // Clean up zombie process
            int pid = proc->pid;
            process_destroy(proc);
            return pid;
        }
    }
    
    // Block until a child exits
    current->state = PROCESS_STATE_BLOCKED;
    process_yield();
    return -1;
}

int sys_getpid(void) {
    process_t* current = process_get_current();
    return current ? current->pid : -1;
}

int sys_kill(int pid, int sig) {
    if (pid <= 0 || pid >= MAX_PROCESSES) return -1;
    
    process_t* target = processes[pid];
    if (!target) return -1;
    
    // For now, any signal just terminates the process
    process_destroy(target);
    return 0;
}