#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "kernel.h"
#include "handlejob.h"
#include "../include/parsejob.h"
#include "shell.h"


#include "token.h"

int handleJob(
    char ***commands,
    int commandCount,
    int index,
    job *job,
    int *currentPgId
){

    pid_t childPid = -1;

    // check if this is a nice command
    bool isNice = 0;
    int offset;
    int priority;

    if (strcmp(commands[index][0], "nice") == 0) {
        isNice = 1;
        offset = 2;
        priority = atoi(commands[index][1]);
    } else {
        offset = 0;
    }

    char ***copy = getCopyOfCommands(commands, commandCount);

    if (copy == NULL) {
        printf("Failed to copy commands\n");
        return -1;
    }

    char *key = copy[index][offset];
    if (strcmp(key, "sleep") == 0) {
        childPid = p_spawn(createSleep, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "zombify") == 0) {
        childPid = p_spawn(zombify, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "orphanify") == 0) {
        childPid = p_spawn(orphanify, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "busy") == 0) {
        childPid = p_spawn(busy, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "ps") == 0) {
        childPid = p_spawn(ps, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "kill") == 0) {
        childPid = p_spawn(killer, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "head") == 0) {
        childPid = p_spawn(head, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "ls") == 0) {
        childPid = p_spawn(ls, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "touch") == 0) {
        childPid = p_spawn(touch, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "mv") == 0) {
        childPid = p_spawn(mv, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "cp") == 0) {
        childPid = p_spawn(cp, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "rm") == 0) {
        childPid = p_spawn(rm, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "cat") == 0) {
        childPid = p_spawn(cat, &copy[index][offset], job->infile, job->outfile);
    } else if (strcmp(key, "chmod") == 0) {
        childPid = p_spawn(chmod, &copy[index][offset], job->infile, job->outfile);
    } else {
        return 0;
    }

    if (childPid == -1) {
        return -1;
    }

    if (isNice) {
        p_nice(childPid, priority);
    }

    job->pgId = childPid;
    job->pids[index] = childPid;

    
    return childPid;
}