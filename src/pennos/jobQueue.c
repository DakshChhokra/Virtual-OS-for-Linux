#include <stdlib.h>
#include <stdio.h>

#include "jobQueue.h"

jobQueue *jobQueueInit() {
    // allocate memory for this jobQueue
    jobQueue *newjobQueue = malloc(sizeof(jobQueue));

    // check if malloc failed
    if (newjobQueue == NULL) {
        return NULL;
    }

    // initialize variables
    newjobQueue->count = 0;
    newjobQueue->front = NULL;
    newjobQueue->back = NULL;
    return newjobQueue;
};

job *jobQueueFront(jobQueue *q) {
    // check if the input jobQueue is initialized
    if (q == NULL) {
        return NULL;
    }

    // check if the jobQueue is empty
    if (q->count == 0 || q->front == NULL || q->back == NULL) {
        return NULL;
    } 

    // return the job at the front of the jobQueue
    return q->front;
}

job *jobQueuePush(jobQueue *q, job *j) {
    // check if the input jobQueue is initialized
    if (q == NULL) {
        return NULL;
    }

    // check if passed in job is null
    if (j == NULL) {
        return NULL;
    }

    // check if the jobQueue is empty
    if (q->count == 0 || q->front == NULL || q->back == NULL) {
        // set front element and back element to elt
        q->front = j;
        q->back = j;
        j->jobId = 1;

    } else {
        // set j prev pointer to back elt and previous back element's next pointer to j
        j->prev = q->back;
        q->back->next = j;
        // set back element to new elt
        q->back = j;
        j->jobId = j->prev->jobId + 1;
    }

    // increment job counter
    q->count = q->count + 1;

    // set the job's next pointer to NULL
    j->next = NULL;

    // return the job
    return j;
}

job *jobQueuePop(jobQueue *q) {
    // check if the input jobQueue is initialized
    if (q == NULL) {
        return NULL;
    }

    // check if the jobQueue is empty
    if (q->count == 0 || q->front == NULL || q->back == NULL) {
        return NULL;
    }

    // get job to pop
    job *j = q->front;

    // update front pointer and front's prev pointer and count
    q->front = j->next;

    // set new front's prev pointer to null if front is not null
    if (q->front != NULL) {
        q->front->prev = NULL;
    }
    q->count = q->count - 1;

    // set the job's next and prev pointers to NULL
    j->prev = NULL;
    j->next = NULL;

    // return the job
    return j;
}

job *jobQueueRemoveJob(jobQueue *q, job *j) {
    // check if the input jobQueue and j are initialized
    if (q == NULL || j == NULL) {
        return NULL;
    }

    // check if the jobQueue is empty
    if (q->count == 0 || q->front == NULL || q->back == NULL) {
        return NULL;
    }

    // check if j at the front
    if (q->front == j) {
        q->front = j->next;
        if (q->front != NULL)
            q->front->prev = NULL;
    }

    // check if j is at the back
    if (q->back == j) {
        q->back = j->prev;
        if (q->back != NULL)
            q->back->next = NULL;
    }

    // always occurs if j is not at the ends
    if (j->prev != NULL && j->next != NULL) {
        j->prev->next = j->next;
        j->next->prev = j->prev;
    }

    // update count
    q->count = q->count - 1;

    // set the job's next and prev pointers to NULL
    j->prev = NULL;
    j->next = NULL;

    // return the job
    return j;
}

void jobQueueClear(jobQueue *q) {
    // check if the input jobQueue is initialized
    if (q != NULL) {
        // free all elements and job pointers
        while (q->front) {
            freeJob(jobQueuePop(q));
        }
        // reset front and back pointers
        q->front = NULL;
        q->back = NULL;
    }
}

void jobQueueDestroy(jobQueue *q) {
    // check if the input jobQueue is initialized
    if (q != NULL) {
        // clear the jobQueue and free the pointer
        jobQueueClear(q);
        free(q);
    }
}

void jobQueuePrint(jobQueue *q) {
    // check if the input jobQueue is initialized
    if (q != NULL) {
        job *current;
        job *next;

        current = q->front;

        // print each job in the jobQueue
        while (current) {
            printJobDetails(current);
            next = current->next;
            current = next;
        }
    } else {
        printf("The job jobQueue is empty.\n");
    }
}

int jobQueueCount(jobQueue *q) {
    // check if the input jobQueue is initialized
    if (q != NULL) {
        return q->count;
    } else {
        // return -1 if the jobQueue is undefined
        return -1;
    }
}