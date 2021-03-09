#include <stdbool.h>
#include "PCB.h"

#ifndef node_HEADER
#define node_HEADER

/**
 * @file node.h
 * @brief Defines a linked process node that contains a PCB and a PID
 */

/**
 * A node with pointers to next/prev nodes to allow a linked-list of nodes
 *
 * Each node has a pid corresponding to some process
 *
 */
typedef struct nodeTag {
    pcb_t *pcb;
    pid_t pid;
    // linked list prev pointer
    struct nodeTag* prev;
    // linked list next pointer
    struct nodeTag* next;
} node;

/**
 * Initialize a new node, with no process group id until first passed into handlenode()
 * @param  pid  pid of the process
 * @return      a pointer to the node
 */
node *newNode(pid_t pid, pcb_t *pcb);

/**
 * Frees a node
 * @param n a node pointer to free
 */
void freenode(node *n);

/**
 * Prints a node
 * @param n a node pointer to print
 */
void printnodeDetails(node *n);

#endif