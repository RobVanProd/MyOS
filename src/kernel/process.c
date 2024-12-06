#include "process.h"
#include "memory.h"
#include "terminal.h"

// Global variables
process_t* current_process = NULL;
static uint32_t next_pid = 1;
static process_t* processes[MAX_PROCESSES] = {NULL};

// Define priority levels
#define PROCESS_PRIORITY_LOW 0
#define PROCESS_PRIORITY_NORMAL 1
#define PROCESS_PRIORITY_HIGH 2

// Define starvation threshold and quantum limits
#define STARVATION_THRESHOLD 1000  // Ticks before priority boost
#define MAX_QUANTUM 100           // Maximum time slice
#define MIN_QUANTUM 20            // Minimum time slice

// Process queue for each priority level
static process_t* ready_queues[3] = {NULL, NULL, NULL};  // Low, Normal, High
static uint32_t process_wait_times[MAX_PROCESSES] = {0};

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

    // Add to process list and scheduler
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
    
    // Disable interrupts during context switch
    asm volatile("cli");
    
    process_t* prev = current_process;
    
    // Save current context if there is a previous process
    if (prev && prev->state != PROCESS_STATE_ZOMBIE) {
        // Save CPU registers
        asm volatile(
            "mov %%esp, %0\n"
            "mov %%ebp, %1\n"
            "pushf\n"
            "pop %2\n"
            : "=m"(prev->context.esp),
              "=m"(prev->context.ebp),
              "=m"(prev->context.eflags)
            :
            : "memory"
        );
        
        // Save segment registers
        asm volatile(
            "mov %%cs, %0\n"
            "mov %%ds, %1\n"
            "mov %%es, %2\n"
            "mov %%fs, %3\n"
            "mov %%gs, %4\n"
            : "=m"(prev->context.cs),
              "=m"(prev->context.ds),
              "=m"(prev->context.es),
              "=m"(prev->context.fs),
              "=m"(prev->context.gs)
            :
            : "memory"
        );
        
        // Save FPU state if used
        if (prev->flags & PROCESS_FLAG_FPU) {
            asm volatile("fxsave %0" : "=m"(prev->fpu_state));
        }
    }
    
    // Update process states
    if (prev) {
        if (prev->state == PROCESS_STATE_RUNNING) {
            prev->state = PROCESS_STATE_READY;
        }
        prev->cpu_time += get_timer_ticks() - prev->last_switch;
    }
    
    next->state = PROCESS_STATE_RUNNING;
    next->last_switch = get_timer_ticks();
    current_process = next;
    
    // Switch page directory if different
    if (!prev || prev->page_directory != next->page_directory) {
        switch_page_directory(next->page_directory);
    }
    
    // Restore FPU state if needed
    if (next->flags & PROCESS_FLAG_FPU) {
        asm volatile("fxrstor %0" : : "m"(next->fpu_state));
    }
    
    // Switch kernel stack
    tss_set_kernel_stack(next->kernel_stack_top);
    
    // Load next context
    asm volatile(
        "mov %0, %%esp\n"
        "mov %1, %%ebp\n"
        "push %2\n"
        "popf\n"
        : 
        : "m"(next->context.esp),
          "m"(next->context.ebp),
          "m"(next->context.eflags)
        : "memory"
    );
    
    // Restore segment registers
    asm volatile(
        "mov %0, %%ds\n"
        "mov %1, %%es\n"
        "mov %2, %%fs\n"
        "mov %3, %%gs\n"
        : 
        : "m"(next->context.ds),
          "m"(next->context.es),
          "m"(next->context.fs),
          "m"(next->context.gs)
        : "memory"
    );
    
    // Re-enable interrupts
    asm volatile("sti");
    
    // If returning to user mode, use iret
    if (next->flags & PROCESS_FLAG_USER) {
        asm volatile(
            "pushw %0\n"        // ss
            "pushl %1\n"        // esp
            "pushfl\n"          // eflags
            "pushw %2\n"        // cs
            "pushl %3\n"        // eip
            "iret\n"
            : 
            : "m"(next->context.ss),
              "m"(next->context.esp),
              "m"(next->context.cs),
              "m"(next->context.eip)
            : "memory"
        );
    } else {
        // Kernel mode - just jump
        asm volatile(
            "jmp *%0\n"
            : 
            : "m"(next->context.eip)
            : "memory"
        );
    }
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
    memset(ready_queues, 0, sizeof(ready_queues));
    memset(process_wait_times, 0, sizeof(process_wait_times));
    current_process = NULL;
}

// Add process to scheduler
void scheduler_add_process(process_t* process) {
    if (!process) return;

    // Find empty slot in process array
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i]) {
            processes[i] = process;
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        kprintf("Error: Maximum number of processes reached\n");
        process_destroy(process);
        return;
    }

    // Add to priority queue
    process->state = PROCESS_STATE_READY;
    process->next = NULL;
    process_wait_times[slot] = 0;

    int priority = process->priority;
    if (priority < 0) priority = 0;
    if (priority > 2) priority = 2;

    if (!ready_queues[priority]) {
        ready_queues[priority] = process;
    } else {
        process_t* current = ready_queues[priority];
        while (current->next) {
            current = current->next;
        }
        current->next = process;
    }
}

// Remove process from scheduler
void scheduler_remove_process(process_t* process) {
    if (!process) return;

    // Remove from process array and clear wait time
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == process) {
            processes[i] = NULL;
            process_wait_times[i] = 0;
            break;
        }
    }

    // Remove from priority queue
    int priority = process->priority;
    if (priority < 0) priority = 0;
    if (priority > 2) priority = 2;

    if (ready_queues[priority] == process) {
        ready_queues[priority] = process->next;
    } else {
        process_t* current = ready_queues[priority];
        while (current && current->next != process) {
            current = current->next;
        }
        if (current) {
            current->next = process->next;
        }
    }

    process->state = PROCESS_STATE_ZOMBIE;
    process->next = NULL;

    // If removing current process, schedule next one
    if (process == current_process) {
        process_t* next = scheduler_next_process();
        if (next && next != process) {
            process_switch(next);
        }
    }
}

// Get next process to run using priority scheduling with anti-starvation
process_t* scheduler_next_process(void) {
    static uint32_t last_schedule_time = 0;
    uint32_t current_time = get_timer_ticks();
    
    // Update wait times and check for starvation
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] && processes[i] != current_process && 
            processes[i]->state == PROCESS_STATE_READY) {
            process_wait_times[i] += current_time - last_schedule_time;
            
            // Boost priority if waiting too long
            if (process_wait_times[i] > STARVATION_THRESHOLD && 
                processes[i]->priority < PROCESS_PRIORITY_HIGH) {
                // Remove from current queue
                scheduler_remove_process(processes[i]);
                // Boost priority and re-add
                processes[i]->priority++;
                scheduler_add_process(processes[i]);
                process_wait_times[i] = 0;
            }
        }
    }
    
    last_schedule_time = current_time;

    // Try to find a process in each priority level
    for (int priority = PROCESS_PRIORITY_HIGH; priority >= PROCESS_PRIORITY_LOW; priority--) {
        process_t* candidate = ready_queues[priority];
        while (candidate) {
            if (candidate->state == PROCESS_STATE_READY && candidate != current_process) {
                // Move to end of queue (round robin within priority)
                scheduler_remove_process(candidate);
                scheduler_add_process(candidate);
                return candidate;
            }
            candidate = candidate->next;
        }
    }

    // If no other process found, continue with current process
    return current_process;
}

// Schedule next process
void process_schedule(void) {
    if (!current_process) return;

    // Calculate quantum based on priority
    uint32_t quantum = MAX_QUANTUM;
    switch (current_process->priority) {
        case PROCESS_PRIORITY_HIGH:
            quantum = MAX_QUANTUM;
            break;
        case PROCESS_PRIORITY_NORMAL:
            quantum = (MAX_QUANTUM + MIN_QUANTUM) / 2;
            break;
        case PROCESS_PRIORITY_LOW:
            quantum = MIN_QUANTUM;
            break;
    }

    // Check if quantum expired
    uint32_t current_time = get_timer_ticks();
    if (current_time - current_process->last_switch >= quantum) {
        process_t* next = scheduler_next_process();
        if (next && next != current_process) {
            process_switch(next);
        }
    }
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