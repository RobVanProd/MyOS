# Process Management API

## Overview

The Process Management API provides interfaces for process creation, scheduling, and control in MyOS. It handles process lifecycle, scheduling, and inter-process communication.

## Process Creation and Control

### Process Creation

```c
process_t* process_create(const char* name, void* entry_point, uint8_t priority, uint8_t flags);
void process_destroy(process_t* process);
```

#### process_create()
Creates a new process.

Parameters:
- `name`: Process name (max 32 chars)
- `entry_point`: Process entry point function
- `priority`: Process priority (PRIORITY_LOW/NORMAL/HIGH)
- `flags`: Process flags (PROCESS_FLAG_KERNEL/USER)

Returns: Pointer to new process structure or NULL on failure

#### process_destroy()
Destroys a process and frees its resources.

Parameters:
- `process`: Process to destroy

### Process Control

```c
void process_yield(void);
void process_sleep(uint32_t ms);
void process_wake(process_t* process);
void process_block(process_t* process);
void process_unblock(process_t* process);
```

#### process_yield()
Voluntarily yields CPU to next process.

#### process_sleep()
Puts current process to sleep for specified time.

Parameters:
- `ms`: Milliseconds to sleep

#### process_wake()
Wakes up a sleeping process.

Parameters:
- `process`: Process to wake

#### process_block()
Blocks a process until explicitly unblocked.

Parameters:
- `process`: Process to block

#### process_unblock()
Unblocks a blocked process.

Parameters:
- `process`: Process to unblock

## Process Information

```c
process_t* process_get_current(void);
process_t* process_get_by_pid(uint32_t pid);
void process_set_priority(process_t* process, uint8_t priority);
```

#### process_get_current()
Gets currently running process.

Returns: Pointer to current process

#### process_get_by_pid()
Finds process by PID.

Parameters:
- `pid`: Process ID to find

Returns: Pointer to process or NULL if not found

#### process_set_priority()
Sets process priority.

Parameters:
- `process`: Process to modify
- `priority`: New priority level

## Scheduler Interface

### Scheduler Control

```c
void scheduler_init(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(process_t* process);
void scheduler_tick(void);
process_t* scheduler_next_process(void);
```

#### scheduler_init()
Initializes the scheduler.

#### scheduler_add_process()
Adds process to scheduler.

Parameters:
- `process`: Process to add

#### scheduler_remove_process()
Removes process from scheduler.

Parameters:
- `process`: Process to remove

#### scheduler_tick()
Handles scheduler timer tick.

#### scheduler_next_process()
Gets next process to run.

Returns: Pointer to next process

### Process States

```c
#define PROCESS_STATE_READY     0
#define PROCESS_STATE_RUNNING   1
#define PROCESS_STATE_BLOCKED   2
#define PROCESS_STATE_ZOMBIE    3
```

### Process Priorities

```c
#define PRIORITY_LOW     0
#define PRIORITY_NORMAL  1
#define PRIORITY_HIGH    2
```

### Process Flags

```c
#define PROCESS_FLAG_KERNEL     0x01
#define PROCESS_FLAG_USER       0x02
```

## System Calls

### Process Management

```c
int sys_fork(void);
int sys_exec(const char* path, char* const argv[]);
void sys_exit(int status);
int sys_wait(int* status);
int sys_getpid(void);
int sys_kill(int pid, int sig);
```

#### sys_fork()
Creates copy of current process.

Returns: Child PID to parent, 0 to child, -1 on error

#### sys_exec()
Replaces current process with new program.

Parameters:
- `path`: Program path
- `argv`: Program arguments

Returns: -1 on error, doesn't return on success

#### sys_exit()
Terminates current process.

Parameters:
- `status`: Exit status code

#### sys_wait()
Waits for child process to terminate.

Parameters:
- `status`: Pointer to store exit status

Returns: PID of terminated child or -1 on error

#### sys_getpid()
Gets current process ID.

Returns: Current PID

#### sys_kill()
Sends signal to process.

Parameters:
- `pid`: Target process ID
- `sig`: Signal number

Returns: 0 on success, -1 on error

## Process Context

### Context Structure

```c
typedef struct {
    uint32_t eax, ebx, ecx, edx;    // General purpose registers
    uint32_t esp, ebp;              // Stack registers
    uint32_t esi, edi;              // Index registers
    uint32_t eip;                   // Instruction pointer
    uint32_t eflags;                // CPU flags
    uint32_t cr3;                   // Page directory base
} process_context_t;
```

### Context Operations

```c
void process_switch(process_t* next);
```

#### process_switch()
Switches to new process context.

Parameters:
- `next`: Process to switch to

## Examples

### Creating a Process
```c
// Create kernel process
process_t* proc = process_create("kernel_task", kernel_task_main,
                               PRIORITY_NORMAL, PROCESS_FLAG_KERNEL);
if (proc) {
    scheduler_add_process(proc);
}
```

### Process Synchronization
```c
// Block until resource available
if (!resource_available()) {
    process_block(process_get_current());
}

// Use resource
use_resource();

// Wake waiting process
process_wake(waiting_process);
```

### Fork/Exec Pattern
```c
int pid = sys_fork();
if (pid == 0) {
    // Child process
    char* args[] = {"program", "arg1", NULL};
    sys_exec("/bin/program", args);
    sys_exit(1);  // Only reached on exec failure
} else if (pid > 0) {
    // Parent process
    int status;
    sys_wait(&status);
}
```

## Best Practices

1. Always check process creation return values
2. Clean up resources in process_destroy
3. Use appropriate process priorities
4. Handle process signals properly
5. Avoid priority inversion
6. Prevent deadlocks in IPC
7. Use proper synchronization primitives

## See Also

- [Memory Management API](memory.md)
- [Process Management Documentation](../../core/process/process.md)
- [Scheduling Documentation](../../core/process/scheduling.md)
- [IPC Documentation](../../core/process/ipc.md) 