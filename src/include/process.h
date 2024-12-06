#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <string.h>
#include "memory.h"

// Process states
#define PROCESS_STATE_READY     0
#define PROCESS_STATE_RUNNING   1
#define PROCESS_STATE_BLOCKED   2
#define PROCESS_STATE_ZOMBIE    3

// Process priorities
#define PRIORITY_LOW     0
#define PRIORITY_NORMAL  1
#define PRIORITY_HIGH    2

// Maximum number of processes
#define MAX_PROCESSES 64

// Process flags
#define PROCESS_FLAG_KERNEL     0x01
#define PROCESS_FLAG_USER       0x02

// Page flags
#define PAGE_PRESENT    0x001
#define PAGE_WRITE     0x002
#define PAGE_USER      0x004

// Process context structure
typedef struct {
    uint32_t eax, ebx, ecx, edx;    // General purpose registers
    uint32_t esp, ebp;              // Stack registers
    uint32_t esi, edi;              // Index registers
    uint32_t eip;                   // Instruction pointer
    uint32_t eflags;                // CPU flags
    uint32_t cr3;                   // Page directory base
} process_context_t;

// Process structure
typedef struct process {
    uint32_t pid;                   // Process ID
    uint32_t parent_pid;            // Parent process ID
    char name[32];                  // Process name
    uint8_t state;                  // Process state
    uint8_t priority;               // Process priority
    uint8_t flags;                  // Process flags
    process_context_t context;      // CPU context
    void* page_directory;           // Page directory
    uint32_t stack_base;            // Base of kernel stack
    uint32_t stack_size;            // Size of kernel stack
    uint32_t heap_base;            // Base of process heap
    uint32_t heap_size;            // Size of process heap
    struct process* next;           // Next process in queue
} process_t;

// Memory management functions
void* get_kernel_page_directory(void);
void* create_page_directory(void);
void free_page_directory(void* page_dir);
uint32_t allocate_region(void* page_dir, uint32_t start, uint32_t size, uint32_t flags);

// Process management functions
void process_init(void);
process_t* process_create(const char* name, void* entry_point, uint8_t priority, uint8_t flags);
void process_destroy(process_t* process);
void process_switch(process_t* next);
void process_yield(void);
void process_sleep(uint32_t ms);
void process_wake(process_t* process);
void process_block(process_t* process);
void process_unblock(process_t* process);
process_t* process_get_current(void);
process_t* process_get_by_pid(uint32_t pid);
void process_set_priority(process_t* process, uint8_t priority);

// Scheduler functions
void scheduler_init(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(process_t* process);
void scheduler_tick(void);
process_t* scheduler_next_process(void);

// System calls
int sys_fork(void);
int sys_exec(const char* path, char* const argv[]);
void sys_exit(int status);
int sys_wait(int* status);
int sys_getpid(void);
int sys_kill(int pid, int sig);

#endif