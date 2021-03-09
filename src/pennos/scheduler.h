#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"
#include "node.h"

/**
 * @file scheduler.h
 * @brief Contains the scheduler runqueues as well as functions to interact with a scheduler
 */

typedef struct {
    int quantaCount;
    queue *high;
    queue *med;
    queue *low;
} scheduler;

/**
 * @brief      Initialize a scheduler
 *
 * @return     Pointer to a new scheduler in memory, or NULL if it failed to initialize
 */
scheduler *schedulerInit();

/**
 * @brief      Adds a process to the scheduler
 *
 * @param      process  Pointer to the process
 * @param      s        Pointer to the scheduler
 */
void addToScheduler(node *process, scheduler *s);

/**
 * @brief      Removes a process from the scheduler
 *
 * @param      process  Pointer to the process
 * @param      s        Pointer to the scheduler
 */
void removeFromScheduler(node *process, scheduler *s);


/**
 * @brief      Gets the next process the scheduler wants to run
 *
 * @param      s     Pointer to the scheduler
 *
 * @return     The next process to run, returns NULL if idle
 */
node *getNextProcess(scheduler *s);
#endif