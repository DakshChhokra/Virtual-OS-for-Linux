#ifndef QUEUE_HEADER
#define QUEUE_HEADER

#include "node.h"

/**
 * @file queue.h
 * @brief Defines a FIFO doubly-linked queue and provides functions for interacting with queues
 */

/**
 * a FIFO node queue
 * the function caller is responsible for freeing the queue pointer after calling queueInit()
 * the function caller is responsible for freeing node pointers from queuePop() and queueRemovenode()
 */
typedef struct {
    int count;
    node *front;
    node *back;
} queue;


/**
 * Allocates and initializes an empty queue
 * @return a pointer to the newly created queue, or null if failed to allocate memory
 */
queue *queueInit();

/**
 * Returns the number of items in the queue
 * @param  q a pointer to the node queue
 * @return   the number of nodes in the queue of -1 if the queue is invalid
 */
int queueCount(queue *q);

/**
 * Returns the node at the front of the queue
 * @param  q a pointer to the queue
 * @return   a pointer to the node at the front of the node queue, or null if the queue is uninitialized/empty
 */
node *queueFront(queue *q);

/**
 * Adds a node to the end of the queue
 * @param  q a pointer to the queue
 * @param  n a pointer to the node to add
 * @return   a pointer to the node just added, or null if the queue is uninitialized
 */
node *queuePush(queue *q, node *n);

/**
 * Removes a node from the front of the queue
 * @param  q a pointer to the queue
 * @return   a pointer to the node just removed, or null if the queue is uninitialized/empty
 */
node *queuePop(queue *q);

/**
 * Finds a specific node with a given pid
 * @param n      the node to find
 * @return the node with the input node's pid, or NULL if it doesn't exist
 */
node *queueSearch(queue *q, node *n);

/**
 * Removes a specific particular node from a particular queue
 * @param  q a pointer to the queue
 * @param  n the pointer to the node in the queue to remove
 * @return pointer to the removed node, or NULL if it doesn't exist
 */
node *queueRemoveNode(queue *q, node *n);

/**
 * Frees and removes all nodes in the queue
 * @param q a pointer to the queue
 */
void queueClear(queue *q);

/**
 * Frees and removes all nodes in the node queue and frees the queue itself
 * @param q a pointer to the queue
 */
void queueDestroy(queue *q);

/**
 * Prints out the nodes of this queue in order to stdout
 * @param q       a pointer to the queue
 */
void queuePrint(queue *q);

#endif