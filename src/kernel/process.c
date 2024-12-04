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
    strcpy(kernel_process->name, "kernel");
    kernel_process->state = PROCESS_STATE_RUNNING;
    kernel_process->priority = PRIORITY_HIGH;
    kernel_process->flags = PROCESS_FLAG_KERNEL;
    kernel_process->next = NULL;
    
    // Set up kernel process memory
    kernel_process->page_directory = get_kernel_page_directory();
    kernel_process->stack_base = 0xC0000000;
    kernel_process->stack_size = 0x4000;
    kernel_process->heap_base = 0xD0000000;
    kernel_process->heap_size = 0x400000;
    
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
    strcpy(process->name, name);
    process->state = PROCESS_STATE_READY;
    process->priority = priority;
    process->flags = flags;
    process->next = NULL;
    
    // Set up process memory
    process->page_directory = create_page_directory();
    if (!process->page_directory) {
        kfree(process);
        return NULL;
    }
    
    // Allocate stack
    process->stack_size = 0x4000;
    process->stack_base = allocate_region(process->page_directory,
                                        0xBFFFF000 - process->stack_size,
                                        process->stack_size,
                                        PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    
    // Set up initial context
    process->context.eip = (uint32_t)entry_point;
    process->context.esp = process->stack_base + process->stack_size;
    process->context.ebp = process->context.esp;
    process->context.eflags = 0x202; // IF flag set
    process->context.cr3 = (uint32_t)process->page_directory;
    
    // Add to process list and array
    processes[process->pid] = process;
    scheduler_add_process(process);
    
    return process;
}

// Destroy a process
void process_destroy(process_t* process) {
    if (!process) return;
    
    // Remove from scheduler
    scheduler_remove_process(process);
    
    // Free process memory
    free_page_directory(process->page_directory);
    
    // Remove from process array
    processes[process->pid] = NULL;
    
    // Free process structure
    kfree(process);
}

// Switch to another process
void process_switch(process_t* next) {
    if (!next || next == current_process) return;
    
    // Save current context
    asm volatile(
        "mov %%esp, %0\n"
        "mov %%ebp, %1\n"
        : "=m"(current_process->context.esp),
          "=m"(current_process->context.ebp)
    );
    
    process_t* prev = current_process;
    current_process = next;
    
    // Load new context
    asm volatile(
        "mov %0, %%cr3\n"
        "mov %1, %%esp\n"
        "mov %2, %%ebp\n"
        "push %3\n"          // eflags
        "push %4\n"          // cs
        "push %5\n"          // eip
        "iret\n"
        :
        : "r"(next->context.cr3),
          "r"(next->context.esp),
          "r"(next->context.ebp),
          "r"(next->context.eflags),
          "i"(0x08),        // kernel code segment
          "r"(next->context.eip)
        : "memory"
    );
}

// Yield CPU to another process
void process_yield(void) {
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
    // Set up timer for preemptive multitasking
    // This will be implemented when we add timer support
}

// Add process to scheduler
void scheduler_add_process(process_t* process) {
    if (!process) return;
    
    // Add to end of process list
    if (!process_list) {
        process_list = process;
    } else {
        process_t* p = process_list;
        while (p->next) p = p->next;
        p->next = process;
    }
}

// Remove process from scheduler
void scheduler_remove_process(process_t* process) {
    if (!process) return;
    
    // Remove from process list
    if (process_list == process) {
        process_list = process->next;
    } else {
        process_t* p = process_list;
        while (p && p->next != process) p = p->next;
        if (p) p->next = process->next;
    }
}

// Scheduler tick handler
void scheduler_tick(void) {
    if (!current_process) return;
    
    // Simple round-robin scheduling
    process_yield();
}

// Get next process to run
process_t* scheduler_next_process(void) {
    if (!process_list) return NULL;
    
    // Find next ready process
    process_t* p = current_process ? current_process->next : process_list;
    if (!p) p = process_list;
    
    while (p != current_process) {
        if (p->state == PROCESS_STATE_READY) return p;
        p = p->next;
        if (!p) p = process_list;
    }
    
    return NULL;
}

// System call implementations
int sys_fork(void) {
    // TODO: Implement fork()
    return -1;
}

int sys_exec(const char* path, char* const argv[]) {
    // TODO: Implement exec()
    return -1;
}

void sys_exit(int status) {
    process_destroy(current_process);
    process_yield(); // Never returns
}

int sys_wait(int* status) {
    // TODO: Implement wait()
    return -1;
}

int sys_getpid(void) {
    return current_process ? current_process->pid : 0;
}

int sys_kill(int pid, int sig) {
    // TODO: Implement kill()
    return -1;
} 