#ifndef PCB_HEADER
#define PCB_HEADER

#include <ucontext.h>
#include <sys/types.h>
#include <stdbool.h>

/**
 * @file PCB.h
 * @brief Defines a PCB type
 */

/**
 * Process Control Block child object
 */
typedef struct childTag {
    pid_t pid;
    struct childTag *prev;
    struct childTag *next;
} child;

/**
 * A process control block, which is used by the kernel to context switch to and
 * from a particular process, send signals to a process or process group, etc.
 */
typedef struct pcbType {
    ucontext_t context; // The context of this process
    int status; // The status of this process (READY, BLOCKED, STOPPED, SIGNALED, EXITED)
    int prevStatus; // The previous state of this process
    int priority_level; // The priority level of this process (HIGH (-1), MEDIUM (0), LOW(1))
    pid_t pid; // The process ID
    pid_t ppid; // The parent process group ID
    pid_t pgid; // The process group ID
    int ticksLeft; // The number of ticks left for a sleep process, -1 for other processes
    bool waitedOn; // Whether this process has been waited on
    child *child_pids; // The children of this process
    child *zombies; // The zombie children of this process
    char *name; // The name of this process
    int stdin; // The file descriptor mapped to stdin for this process
    int stdout; // The file descriptor mapped to stdout for this process
} pcb_t;

#endif