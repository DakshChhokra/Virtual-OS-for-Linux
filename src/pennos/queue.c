#include <stdlib.h>
#include <stdio.h>

#include "queue.h"

#define FAILURE -1

queue *queueInit() {
    // allocate memory for this queue
    queue *newQueue = malloc(sizeof(queue));

    // check if malloc failed
    if (newQueue == NULL) {
        return NULL;
    }

    // initialize variables
    newQueue->count = 0;
    newQueue->front = NULL;
    newQueue->back = NULL;
    return newQueue;
};

node *queueFront(queue *q) {
    // check if the input queue is initialized
    if (q == NULL) {
        return NULL;
    }

    // check if the queue is empty
    if (q->count == 0 || q->front == NULL || q->back == NULL) {
        return NULL;
    } 

    // return the pid at the front of the queue
    return q->front;
}

node *queuePush(queue *q, node *n) {
    // check if the input queue is initialized
    if (q == NULL) {
        return NULL;
    }

    // check if passed in node is null
    if (n == NULL) {
        return NULL;
    }

    // check if the queue is empty
    if (q->count == 0 || q->front == NULL || q->back == NULL) {
        // set front element and back element to elt
        q->front = n;
        q->back = n;

    } else {
        // set n prev pointer to back elt and previous back element's next pointer to n
        n->prev = q->back;
        q->back->next = n;
        // set back element to new elt
        q->back = n;
    }

    // increment node counter
    q->count = q->count + 1;

    // set the node's next pointer to NULL
    n->next = NULL;

    // return the node
    return n;
}

node *queuePop(queue *q) {
    // check if the input queue is initialized
    if (q == NULL) {
        return NULL;
    }

    // check if the queue is empty
    if (q->count == 0 || q->front == NULL || q->back == NULL) {
        return NULL;
    }

    // get node to pop
    node *n = q->front;

    // update front pointer and front's prev pointer and count
    q->front = n->next;

    // set new front's prev pointer to null if front is not null
    if (q->front != NULL) {
        q->front->prev = NULL;
    }
    q->count = q->count - 1;

    // set the node's next and prev pointers to NULL
    n->prev = NULL;
    n->next = NULL;

    // return the node
    return n;
}

node *queueSearch(queue *q, node *n) {
    // iterate through the queue until the desired 
    // node is found or the end of the queue is reached
    node *curr = q->front;
    while (curr != NULL && curr->pid != n->pid) {
        curr = curr->next;
    }

    return curr;
}

node *queueRemoveNode(queue *q, node *n) {
    // check if the input queue and n are initialized
    if (q == NULL || n == NULL) {
        return NULL;
    }

    // check if the queue is empty
    if (q->count == 0 || q->front == NULL || q->back == NULL) {
        return NULL;
    }

    // check if n at the front
    if (q->front == n) {
        q->front = n->next;
        if (q->front != NULL)
            q->front->prev = NULL;
    }

    // check if n is at the back
    if (q->back == n) {
        q->back = n->prev;
        if (q->back != NULL)
            q->back->next = NULL;
    }

    // always occurs if n is not at the ends
    if (n->prev != NULL && n->next != NULL) {
        n->prev->next = n->next;
        n->next->prev = n->prev;
    }

    // update count
    q->count = q->count - 1;

    // set the node's next and prev pointers to NULL
    n->prev = NULL;
    n->next = NULL;

    // return the node
    return n;
}

void queueClear(queue *q) {
    // check if the input queue is initialized
    if (q != NULL) {
        // free all elements and node pointers
        while (q->front) {
            freenode(queuePop(q));
        }
        // reset front and back pointers
        q->front = NULL;
        q->back = NULL;
    }
}

void queueDestroy(queue *q) {
    // check if the input queue is initialized
    if (q != NULL) {
        // clear the queue and free the pointer
        queueClear(q);
        free(q);
    }
}

void queuePrint(queue *q) {
    // check if the input queue is initialized
    if (q != NULL) {
        node *current;
        node *next;

        current = q->front;

        // print each node in the queue
        while (current) {
            // printnodeDetails(current);
            next = current->next;
            current = next;
        }
    } else {
        printf("The node queue is empty.\n");
    }
}

int queueCount(queue *q) {
    // check if the input queue is initialized
    if (q != NULL) {
        return q->count;
    } else {
        // return -1 if the queue is undefined
        return FAILURE;
    }
}