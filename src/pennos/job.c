#include <stdlib.h>
#include <stdio.h>

#include "token.h"
#include "job.h"

job *newJob(char ***jobCommands, int commandCount, int infile, int outfile) {
    // allocate memory for this job
    job *result = malloc(sizeof(job));

    char *jobDesc = getCommandStringFromTokens(jobCommands, commandCount);

    // check if malloc failed
    if (result == NULL) {
        return NULL;
    } else {
        // set job details
        result->jobId = 0;
        result->pgId = -1;
        result->jobDesc = jobDesc;
        result->commandCount = commandCount;
        result->processesFinished = 0;

        // allocate memory for children pids and pipes
        int *pids = malloc(sizeof(pid_t) * commandCount);

        if (pids == NULL) {
            free(result);
            return NULL;
        }

        int (*pipes)[2] = NULL;

        if (commandCount > 1) {
            pipes = malloc(sizeof(int[2]) * (commandCount - 1));
            if (pipes == NULL) {
                free(pids);
                free(result);
                return NULL;
            }
        }

        result->pids = pids;
        result->pipes = pipes;

        result->infile = infile;
        result->outfile = outfile;

        // by default a job starts running
        result->isRunning = true;

        // initialize prev/next pointers
        result->next = NULL;
        result->prev = NULL;

        return result;
    }
}

void freeJob(job* j) {
    if (j != NULL) {
        free(j->jobDesc);
        free(j->pids);
        if (j->pipes != NULL)
            free(j->pipes);
        free(j);
    }
}

void printJobDetails(job* j) {
    char *status;
    if (j->isRunning) {
        status = "running";
    } else {
        status = "stopped";
    }
    printf("[%d] %s (%s)\n", j->jobId, j->jobDesc, status);
}

void printRunningJob(job *j) {
    printf("Running: %s\n", j->jobDesc);
}

void printFinishedJob(job *j) {
    printf("Finished: %s\n", j->jobDesc);
}