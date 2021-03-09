#include "job.h"

#ifndef jobQueue_HEADER
#define jobQueue_HEADER

/**
 * @file jobQueue.h
 * @brief Contains a linked-list style job queue implementation
 */

/**
 * A FIFO jobQueue
 * the function caller is responsible for freeing the jobQueue pointer after calling jobQueueInit()
 * the function caller is responsible for freeing job pointers from jobQueuePop() and jobQueueRemoveJob()
 */
typedef struct {
    int count;
    job *front;
    job *back;
} jobQueue;


/**
 * Allocates and initializes an empty jobQueue
 * @return a pointer to the newly created jobQueue, or null if failed to allocate memory
 */
jobQueue *jobQueueInit();

/**
 * Returns the number of items in the jobQueue
 * @param  q a pointer to the jobQueue
 * @return   the number of jobs in the jobQueue of -1 if the jobQueue is invalid
 */
int jobQueueCount(jobQueue *q);

/**
 * Returns the job at the front of the jobQueue
 * @param  q a pointer to the jobQueue
 * @return   a pointer to the job at the front of the jobQueue, or null if the jobQueue is uninitialized/empty
 */
job *jobQueueFront(jobQueue *q);

/**
 * Adds a job to the end of the jobQueue
 * @param  q a pointer to the jobQueue
 * @param  j a pointer to the job to add
 * @return   a pointer to the job just added, or null if the jobQueue is uninitialized
 */
job *jobQueuePush(jobQueue *q, job *j);

/**
 * Removes a job from the front of the jobQueue
 * @param  q a pointer to the jobQueue
 * @return   a pointer to the job just removed, or null if the jobQueue is uninitialized/empty
 */
job *jobQueuePop(jobQueue *q);

/**
 * Removes a specific particular job from a particular jobQueue
 * @param  q a pointer to the jobQueue
 * @param  j the pointer to the job in the jobQueue to remove
 */
job *jobQueueRemoveJob(jobQueue *q, job *j);

/**
 * Frees and removes all jobs in the jobQueue
 * @param q a pointer to the jobQueue
 */
void jobQueueClear(jobQueue *q);

/**
 * Frees and removes all jobs in the jobQueue and frees the jobQueue itself
 * @param q a pointer to the jobQueue
 */
void jobQueueDestroy(jobQueue *q);

/**
 * Prints out the jobs of this jobQueue in order to stdout
 * @param q       a pointer to the jobQueue
 */
void jobQueuePrint(jobQueue *q);

#endif