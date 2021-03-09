#ifndef KERNEL_HEADER
#define KERNEL_HEADER

#include "PCB.h"
#include "queue.h"
#include "../fs/fat.h"
#include "scheduler.h"

/**
 * @file kernel.h
 * @brief Contains kernel level functions
 */

// Global variable for the mounted file system
fat *mountedFat;

/*
 * Kernel level function for creating a new child process and adding it to the process table 
 * @param parent a pointer the pcb of the parent
 * @return a pointer to new pcb
 */
pcb_t *k_process_create(pcb_t *parent);

/*
 * Function for zombiefying a given process
 * @param process, pointer to the process 
 */
void dealWithUnwaitedProcess(pcb_t *process);

/*
 * Kernel level function for killing the process referenced by process with the signal
 * @param PCB pointer to the PCB of the process to kill
 * @param signal the value corresponding to the signal to be passed
 */
void k_process_kill(pcb_t *process, int signal);

/*
 * Kernel level function for cleaning up the resources of a process when it is 
 * finished/terminated.
 * @param process a pointer to the PCB we need to clean
 */
void k_process_cleanup(pcb_t *process);

/*
 * Function for terminating and cleaning all children and zombies of a given parent process
 * @param process, pointer to the parent process 
 */
void clearZombiesAndChildren(pcb_t *process);

/* 
 * Function used for the scheduler context
 */
void schedule();

/*
 * Getter function for getting the process currently at the foreground
 * @return the pointer to the pcb of the foreground process
 */
pcb_t *getForegroundProcess();

/*
 * Getter function for getting the currently running process
 * @return the pointer to the pcb of the currently running process
 */
pcb_t *getCurrProcess();

/*
 * Getter function for getting the process table
 * @return the pointer to the process table
 */
queue *getProcessTable();

/*
 * Getter function for getting the current number of ticks that have passed
 * @return the number of ticks that have passed
 */
int getNumTicks();

/*
 * Getter function for getting the scheduler
 * @return the pointer to the scheduler
 */
scheduler *getScheduler();

/*
 * Function for creating a context using the given function and arguments
 * @param ucp, pointer to the ucontext_t struct which will store the new context
 * @param func, pointer to the function to be associated with the new context
 * @param argv, pointer to array of arguments to be passed to func
 */
void makeContext(ucontext_t *ucp,  void (*func)(), char *argv[]);

/*
 * Forces a context switch which runs the scheduler's next process, or the idle process if no process is ready.
 */
void switchContext(int signum);

/*
 * Function for adding a process to the sleep queue
 * @param n, pointer to node containing the sleep process
 */
void addToAsleep(node *n);

/*
 * Function for setting the foreground process
 * @param pid, the pid of the process to set in the foreground
 */
void setForeground (pid_t pid);

/*
 * Function for unblocking a parent process, used when a child that has 
 * been waited on changes state
 * @param ppid, the parent pid
 */
void unblockParent(pid_t ppid);

/*
 * Getter function for getting the log file
 */
FILE *getLogfile();

/*
 * Helper method for recursively traversing the zombies and children of each 
 * (grand)child of a given process. Used only by p_waitpid with pid<=0
 * @param process, pointer to the parent process
 * @param wstatus, pointer to the status variable for the process to be found
 * @return the pid of the process whose status was changed
 */
pid_t traverseChild(pcb_t *process, int *wstatus);

#endif