#include <stdlib.h>
#include <stdio.h>
#include "node.h"
#include "PCB.h"

node *newNode(pid_t pid, pcb_t *pcb) {
    
    // allocate memory for this node
    node *result = malloc(sizeof(node));

    // check if malloc failed
    if (result == NULL) {
        return NULL;
    }

    result->pid = pid;
    result->pcb = pcb;

    // initialize prev/next pointers
    result->next = NULL;
    result->prev = NULL;
    return result;
}

void freenode(node* n) {
    if (n != NULL) {
        free(n);
    }
}

void printnodeDetails(node* n) {
    printf("pid: %d\n", n->pid);
}