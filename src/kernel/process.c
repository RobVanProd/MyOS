#include "process.h"
#include "kheap.h"
#include "memory.h"
#include "terminal.h"
#include "paging.h"

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
    if (!kernel_process) {
        kprintf("Failed to allocate kernel process\n");
        return;
    }

    // Initialize kernel process
    memset(kernel_process, 0, sizeof(process_t));
    strncpy(kernel_process->name, "kernel", MAX_PROCESS_NAME - 1);
    kernel_process->pid = 0;
    kernel_process->state = PROCESS_STATE_RUNNING;
    kernel_process->priority = PROCESS_PRIORITY_HIGH;
    kernel_process->flags = PROCESS_FLAG_KERNEL;
    kernel_process->page_directory = get_kernel_page_directory();
    
    // Set as current process
    current_process = kernel_process;
}

// Create a new process
process_t* process_create(const char* name, void (*entry)(void)) {
    // Allocate process structure
    process_t* process = kmalloc(sizeof(process_t));
    if (!process) {
        kprintf("Failed to allocate process structure\n");
        return NULL;
    }

    // Initialize process structure
    memset(process, 0, sizeof(process_t));
    strncpy(process->name, name, MAX_PROCESS_NAME - 1);
    
    // Create page directory
    process->page_directory = create_page_directory();
    if (!process->page_directory) {
        kprintf("Failed to create page directory\n");
        kfree(process);
        return NULL;
    }

    // Set up process context
    process->context.eip = (uint32_t)entry;
    process->context.eflags = 0x202;  // IF flag set
    process->context.cs = 0x08;       // Kernel code segment
    process->context.ds = 0x10;       // Kernel data segment
    process->context.es = 0x10;
    process->context.fs = 0x10;
    process->context.gs = 0x10;
    process->context.ss = 0x10;
    
    // Allocate kernel stack
    process->stack_size = 8192;  // 8KB stack
    process->stack = (uint32_t)kmalloc(process->stack_size);
    if (!process->stack) {
        kprintf("Failed to allocate kernel stack\n");
        kfree(process->page_directory);
        kfree(process);
        return NULL;
    }
    
    process->context.esp = process->stack + process->stack_size;
    process->context.ebp = process->context.esp;
    
    // Initialize other fields
    process->pid = next_pid++;
    process->state = PROCESS_STATE_READY;
    process->priority = PROCESS_PRIORITY_NORMAL;
    process->flags = PROCESS_FLAG_USER;
    process->parent = current_process;
    
    // Add to process list
    scheduler_add_process(process);
    
    return process;
}

// Destroy a process
void process_destroy(process_t* process) {
    if (!process) return;

    // Remove from scheduler
    scheduler_remove_process(process);

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
    process_t* prev = current_process;
    
    // Save current context if there is one
    if (prev) {
        // Save general purpose registers
        asm volatile(
            "mov %%esp, %0\n"
            "mov %%ebp, %1\n"
            "pushf\n"
            "pop %2\n"
            : "=r"(prev->context.esp),
              "=r"(prev->context.ebp),
              "=r"(prev->context.eflags)
            :
            : "memory"
        );
        
        // Save segment registers
        asm volatile(
            "mov %%cs, %%ax\n"
            "mov %%ax, %0\n"
            "mov %%ds, %%ax\n"
            "mov %%ax, %1\n"
            "mov %%es, %%ax\n"
            "mov %%ax, %2\n"
            "mov %%fs, %%ax\n"
            "mov %%ax, %3\n"
            "mov %%gs, %%ax\n"
            "mov %%ax, %4\n"
            "mov %%ss, %%ax\n"
            "mov %%ax, %5\n"
            : "=m"(prev->context.cs),
              "=m"(prev->context.ds),
              "=m"(prev->context.es),
              "=m"(prev->context.fs),
              "=m"(prev->context.gs),
              "=m"(prev->context.ss)
            :
            : "ax", "memory"
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
        : "r"(next->context.esp),
          "r"(next->context.ebp),
          "r"(next->context.eflags)
        : "memory"
    );
    
    // Restore segment registers
    asm volatile(
        "mov %0, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %1, %%ax\n"
        "mov %%ax, %%es\n"
        "mov %2, %%ax\n"
        "mov %%ax, %%fs\n"
        "mov %3, %%ax\n"
        "mov %%ax, %%gs\n"
        : 
        : "m"(next->context.ds),
          "m"(next->context.es),
          "m"(next->context.fs),
          "m"(next->context.gs)
        : "ax", "memory"
    );
    
    // Re-enable interrupts
    asm volatile("sti");
    
    // If returning to user mode, use iret
    if (next->flags & PROCESS_FLAG_USER) {
        asm volatile(
            "mov %0, %%ax\n"
            "mov %%ax, %%ds\n"
            "mov %%ax, %%es\n"
            "mov %%ax, %%fs\n"
            "mov %%ax, %%gs\n"
            "push %1\n"        // ss
            "push %2\n"        // esp
            "push %3\n"        // eflags
            "push %4\n"        // cs
            "push %5\n"        // eip
            "iret\n"
            : 
            : "m"(next->context.ds),
              "m"(next->context.ss),
              "r"(next->context.esp),
              "r"(next->context.eflags),
              "m"(next->context.cs),
              "r"(next->context.eip)
            : "ax", "memory"
        );
    } else {
        // Kernel mode - just jump
        asm volatile(
            "jmp *%0\n"
            : 
            : "r"(next->context.eip)
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

// Put a process to sleep
void process_sleep(uint32_t ticks) {
    if (!current_process) return;
    
    current_process->sleep_until = get_timer_ticks() + ticks;
    current_process->state = PROCESS_STATE_SLEEPING;
    
    // Remove from scheduler
    scheduler_remove_process(current_process);
    
    // Force a context switch
    process_yield();
}

// Wake up a sleeping process
void process_wake(process_t* process) {
    if (!process) return;
    
    if (get_timer_ticks() >= process->sleep_until) {
        process->state = PROCESS_STATE_READY;
        process->sleep_until = 0;
        scheduler_add_process(process);
    }
}

// System call implementations
int sys_fork(void) {
    // Create new process structure
    process_t* child = kmalloc(sizeof(process_t));
    if (!child) {
        kprintf("Failed to allocate child process\n");
        return -1;
    }

    // Copy process structure
    memcpy(child, current_process, sizeof(process_t));
    child->pid = next_pid++;
    child->parent = current_process;
    child->state = PROCESS_STATE_READY;
    child->next = NULL;

    // Copy page directory
    child->page_directory = copy_page_directory(current_process->page_directory);
    if (!child->page_directory) {
        kfree(child);
        return -1;
    }

    // Allocate and copy stack
    child->stack = (uint32_t)kmalloc(current_process->stack_size);
    if (!child->stack) {
        kfree(child->page_directory);
        kfree(child);
        return -1;
    }
    memcpy((void*)child->stack, (void*)current_process->stack, current_process->stack_size);

    // Update stack pointers to point to new stack
    uint32_t stack_offset = child->stack - current_process->stack;
    child->context.esp += stack_offset;
    child->stack_base = current_process->stack_base + stack_offset;

    // Set return value for child
    child->context.eax = 0;  // Child gets 0

    // Add to process list and scheduler
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