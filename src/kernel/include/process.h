#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include "kheap.h"
#include "string.h"
#include "memory.h"  // Add this for page_directory_t

// Process states
#define PROCESS_STATE_READY     0
#define PROCESS_STATE_RUNNING   1
#define PROCESS_STATE_BLOCKED   2
#define PROCESS_STATE_ZOMBIE    3

// Process priorities
#define PROCESS_PRIORITY_LOW    0
#define PROCESS_PRIORITY_NORMAL 1
#define PROCESS_PRIORITY_HIGH   2

// Process flags
#define PROCESS_FLAG_KERNEL     0x00000001
#define PROCESS_FLAG_USER       0x00000002

// Maximum process name length
#define MAX_PROCESS_NAME 32
#define MAX_PROCESSES    64

// Process context structure
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
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
    uint8_t flags;                         // Process flags
    process_context_t context;             // CPU context
    uint32_t stack;                        // Kernel stack location
    uint32_t stack_size;                   // Stack size
    uint32_t stack_base;                   // Stack base address
    page_directory_t* page_directory;      // Page directory
    struct process* parent;                // Parent process
    struct process* next;                  // Next process in queue
    uint32_t sleep_until;                  // Wake up time for sleeping processes
} process_t;

// Function declarations
void process_init(void);
process_t* process_create(const char* name, void (*entry)(void));
void process_destroy(process_t* process);
void process_switch(process_t* next);
void process_schedule(void);
void process_sleep(uint32_t ms);
void process_wake(process_t* process);
void process_yield(void);

// Scheduler functions
void scheduler_init(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(process_t* process);
process_t* scheduler_next_process(void);

// Process management functions
process_t* process_get_by_pid(uint32_t pid);
int sys_fork(void);
int sys_exec(const char* path, char* const argv[]);
void sys_exit(int status);
int sys_wait(int* status);
int sys_getpid(void);
int sys_kill(int pid, int sig);

// Global variables
extern process_t* current_process;

#endif // PROCESS_H
