#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include "node.h"
#include "kernel.h"

#define READY 0
#define BLOCKED 1
#define RUNNING 2

int curr = -1;

// iterate through queue returning first running process else return NULL
node *iterate(queue *q) {
    node *currProcess = q->front;
    while (currProcess != NULL && currProcess->pcb->status != READY) {
        currProcess = currProcess->next;
    }
    return currProcess;
}

scheduler *schedulerInit() {

    // allocate memory for scheduler
    scheduler *newScheduler = malloc(sizeof(scheduler));

    // check if malloc failed
    if (newScheduler == NULL) {
        return NULL;
    }

    // initialize variables
    newScheduler->quantaCount = 0;
    newScheduler->high = queueInit();
    newScheduler->med = queueInit();
    newScheduler->low = queueInit();

    return newScheduler;
}

void addToScheduler(node *process, scheduler *s) {
    // get priotiy of process
    int priority = process->pcb->priority_level;

    // add to appropriate queue
    if (priority == -1) {
        queuePush(s->high, newNode(process->pid, process->pcb));
    } else if (priority == 0) {
        queuePush(s->med, newNode(process->pid, process->pcb));
    } else {
        queuePush(s->low, newNode(process->pid, process->pcb));
    }

}

void removeFromScheduler(node *process, scheduler *s) {
    // get priotiy of process
    int priority = process->pcb->priority_level;

    // remove from appropriate queue
    if (priority == -1) {
    	node *n = queueSearch(s->high, process);
        queueRemoveNode(s->high, n);
    } else if (priority == 0) {
    	node *n = queueSearch(s->med, process);
        queueRemoveNode(s->med, n);
    } else {
    	node *n = queueSearch(s->low, process);
        queueRemoveNode(s->low, n);
    }
}

node *getNextProcess(scheduler *s) {

    int quantaCount = s->quantaCount;

    queue *arr[3] = {s->high, s->med, s->low};

    int startIdx = 0;

    if (quantaCount >= 0 && quantaCount < 9)
        startIdx = 0;
    else if (quantaCount >= 9 && quantaCount < 15)
        startIdx = 1;
    else
        startIdx = 2;

    for (int i = 0; i < 3; i++) {
        int newIdx = (startIdx + i) % 3;
        node *currProcess = iterate(arr[newIdx]);

        // check med priority queue
        if (currProcess != NULL) {
            s->quantaCount = (quantaCount + 1) % 19;

            // add a process back to the queue in a round-robin fashion
            removeFromScheduler(currProcess, s);
            addToScheduler(currProcess, s);

            return currProcess;
        }
    }

    // return NULL (idle) since no queues have ready processes
    s->quantaCount = (quantaCount + 1) % 19;
    return NULL;

    // select process from high priority queue to run 
    // if (quantaCount >= 0 && quantaCount < 9) {
    // 	curr = -1;
    //     node *currProcess = iterate(s->high);

    //     // check med priority queue
    //     if (currProcess == NULL) {

    //         // set turn to med priority queue
    //         s->quantaCount = 9;
    //         curr = 0;
    //         currProcess = iterate(s->med);

    //         // check low priority queue
    //         if (currProcess == NULL) {
                
    //             // set turn to low priority queue
    //             s->quantaCount = 15;
    //             currProcess = iterate(s->low);

    //             // idle process
    //             if (currProcess == NULL) {
    //             	s->quantaCount = (quantaCount + 1) % 19;
    //                 printf("returning IDLE process 1\n");
    //                 return NULL;
    //             // return found process
    //             } else {
    //                 s->quantaCount = (quantaCount + 1) % 19;

    //                 // add a process back to the queue in a round-robin fashion
    //                 removeFromScheduler(currProcess, s);
    //                 addToScheduler(currProcess, s);

    //                 return currProcess;
    //             }

    //         // return found process
    //         } else {
    //             s->quantaCount = quantaCount + 1;

    //             // add a process back to the queue in a round-robin fashion
    //             removeFromScheduler(currProcess, s);
    //             addToScheduler(currProcess, s);

    //             return currProcess;
    //         }
        
    //     // return found process
    //     } else {
    //         s->quantaCount = quantaCount + 1;

    //         // add a process back to the queue in a round-robin fashion
    //         removeFromScheduler(currProcess, s);
    //         addToScheduler(currProcess, s);

    //         return currProcess;
    //     }
        
    // // select process from med priority queue to run
    // } else if (quantaCount >= 9 && quantaCount < 15) {

    //     node *currProcess = iterate(s->med);

    //     // check low priority queue
    //     if (currProcess == NULL) {

    //         // set turn to low priority queue
    //         s->quantaCount = 15;
    //         currProcess = iterate(s->low);

    //         // idle process
    //         if (currProcess == NULL) {

    //             // set turn to high priority queue
    //             s->quantaCount = 0;
    //             currProcess = iterate(s->high);

    //             if (currProcess == NULL) {
    //                 s->quantaCount = (quantaCount + 1) % 19;
    //                 printf("returning IDLE process 2\n");
    //                 return NULL;
    //             } else {
    //                 s->quantaCount = (quantaCount + 1) % 19;

    //                 // add a process back to the queue in a round-robin fashion
    //                 removeFromScheduler(currProcess, s);
    //                 addToScheduler(currProcess, s);

    //                 return currProcess;
    //             }

    //         // return found process
    //         } else {
    //             s->quantaCount = (quantaCount + 1) % 19;

    //             // add a process back to the queue in a round-robin fashion
    //             removeFromScheduler(currProcess, s);
	   //          addToScheduler(currProcess, s);

    //             return currProcess;
    //         }

    //     // return found process
    //     } else {
    //         s->quantaCount = (quantaCount + 1) % 19;

    //         // add a process back to the queue in a round-robin fashion
    //         removeFromScheduler(currProcess, s);
    //         addToScheduler(currProcess, s);

    //         return currProcess;
    //     }

    // // select process from low priority queue to run
    // } else {

    //     node *currProcess = iterate(s->low);
    //     // idle process
    //     if (currProcess == NULL) {
    //     	s->quantaCount = (quantaCount + 1) % 19;
    //         printf("returning IDLE process 3\n");
    //         return NULL;
    //     // return found process
    //     } else {
    //         s->quantaCount = (quantaCount + 1) % 19;

    //         // add a process back to the queue in a round-robin fashion
    //         removeFromScheduler(currProcess, s);
    //         addToScheduler(currProcess, s);

    //         return currProcess;
    //     }

    // }
}