#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include "kheap.h"
#include "string.h"
#include "memory.h"
#include "timer.h"
#include "tss.h"

// Process states
#define PROCESS_STATE_RUNNING 1
#define PROCESS_STATE_READY   2
#define PROCESS_STATE_BLOCKED 3
#define PROCESS_STATE_ZOMBIE    4
#define PROCESS_STATE_SLEEPING  5

// Process priorities
#define PROCESS_PRIORITY_LOW    0
#define PROCESS_PRIORITY_NORMAL 1
#define PROCESS_PRIORITY_HIGH   2

// Process flags
#define PROCESS_FLAG_KERNEL     0x00000001
#define PROCESS_FLAG_USER       0x00000002
#define PROCESS_FLAG_FPU        0x00000004

// Maximum process name length
#define MAX_PROCESS_NAME 32
#define MAX_PROCESSES    64

// Process context structure
typedef struct {
    // General purpose registers
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    
    // Segment registers
    uint16_t cs;
    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;
    uint16_t ss;
    
    // Special registers
    uint32_t eip;
    uint32_t eflags;
    uint32_t cr3;  // Page directory physical address
} process_context_t;

// Process structure
typedef struct process {
    uint32_t pid;                          // Process ID
    char name[MAX_PROCESS_NAME];           // Process name
    uint8_t state;                         // Process state
    uint8_t priority;                      // Process priority
    uint32_t flags;                        // Process flags
    process_context_t context;             // CPU context
    void* page_directory;                  // Page directory
    uint32_t stack;                        // Kernel stack location (for compatibility)
    uint32_t stack_size;                   // Size of kernel stack
    uint32_t stack_base;                   // Base of kernel stack
    uint32_t kernel_stack_top;             // Top of kernel stack
    uint32_t user_stack_top;               // Top of user stack
    uint32_t heap_start;                   // Start of process heap
    uint32_t heap_end;                     // End of process heap
    uint32_t cpu_time;                     // CPU time used
    uint32_t last_switch;                  // Last context switch time
    uint32_t sleep_until;                  // Wake up time for sleeping processes
    uint8_t fpu_state[512] __attribute__((aligned(16))); // FPU state
    struct process* parent;                // Parent process
    struct process* next;                  // Next process in list
    struct process* prev;                  // Previous process in list
} process_t;

// External declarations
extern process_t* current_process;

// Function declarations
void process_init(void);
process_t* process_create(const char* name, void (*entry)(void));
void process_destroy(process_t* process);
void process_switch(process_t* next);
void process_yield(void);
void process_sleep(uint32_t ticks);
void process_wake(process_t* process);
process_t* process_get_by_pid(uint32_t pid);

// Scheduler functions
void scheduler_init(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(process_t* process);
process_t* scheduler_next_process(void);
void process_schedule(void);

// System calls
int sys_fork(void);
int sys_exec(const char* path, char* const argv[]);
void sys_exit(int status);
int sys_wait(int* status);
int sys_getpid(void);
int sys_kill(int pid, int sig);

#endif /* PROCESS_H */
