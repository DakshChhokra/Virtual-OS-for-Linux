#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "jobcontrol.h"
#include "../include/macros.h"
#include "shell.h"

bool isJobControlCommand(char **command) {
    char *call = command[0];
    return strcmp(call, "fg") == 0 || strcmp(call, "bg") == 0 || strcmp(call, "jobs") == 0;
}

void handleJobControlCommand(char **command, jobQueue *q, int *currentPgId) {
    char *call = command[0];

    int jobId = -1;

    // see if we are given an argument
    if (command[1] != NULL) {
        jobId = atoi(command[1]);
    }

    job *foundJob = NULL;

    if (jobId == -1) {
        // choose job to be the most recently stopped job, or else the most recently created job
        // initially use the most recently created job
        foundJob = q->back;

        // naively traverse q from the end to see if we can find a stopped job
        job *currJob = q->back;

        while (currJob != NULL) {
            if (!currJob->isRunning) {
                // found the most recently stopped job
                foundJob = currJob;
                break;
            }
            currJob = currJob->prev;
        }
    } else if (jobId > 0) {
        // naively traverse q until we find the desired job
        job *currJob = q->front;

        while (currJob->jobId < jobId) {
            currJob = currJob->next;
        }

        // check if we found the desired job
        if (currJob->jobId == jobId) {
            foundJob = currJob;
        }
    }

    if (strcmp(call, "fg") == 0) {
        if (handleForegroundCommand(q, foundJob, currentPgId) == FAILURE) {
            printf("%s\n", "fg: failed");
        }
    } else if (strcmp(call, "bg") == 0) {
        if (handleBackgroundCommand(q, foundJob) == FAILURE) {
            printf("%s\n", "bg: failed");
        }
    } else if (strcmp(call, "jobs") == 0) {
        if(handleJobsCommand(q) == FAILURE) {
            printf("%s\n", "jobs: failed");
        }
    }
}

int handleForegroundCommand(jobQueue *q, job *job, int *currentPgId) {
    // check if job is valid
    if (job == NULL) {
        return FAILURE;
    }

    if (!job->isRunning) {
        // resume job before bringing the job to the foreground
        printf("Restarting: ");
        job->isRunning = true;
    }

    printf("%s\n", job->jobDesc);

    // remove the job from the jobQueue and bring into the foreground
    if (jobQueueRemoveJob(q, job) == NULL) {
        return FAILURE;
    }

    return putJobInForeground(q, job, true);
}

int handleBackgroundCommand(jobQueue *q, job *job) {
    // check if job is valid
    if (job == NULL) {
        return FAILURE;
    }

    // check if the job is already running
    if (job->isRunning) {
        return FAILURE;
    }

    // send sigcont to the process
    if (p_kill(job->pgId, S_SIGCONT) == -1) {
        return FAILURE;
    }

    // wait for bachground process (done to update the status)
    int waitStatus = 0;
    p_waitpid(job->pgId, &waitStatus, true);

    job->isRunning = true;
    printf("Running: %s\n", job->jobDesc);

    return SUCCESS;
}

int handleJobsCommand(jobQueue *q) {
    // write jobs info
    jobQueuePrint(q);
    return SUCCESS;
}

int putJobInForeground(jobQueue *q, job *job, bool interactive) {
    // send SIGCONT signal to proces group
    if (p_kill(job->pgId, S_SIGCONT) == -1) {
        return FAILURE;
    }

    job->isRunning = true;

    // wait until graceful termination of all processes or killed by signal
    int waitStatus = 0;
    for (int i = 0; i < job->commandCount; i++) {
        waitStatus = 0;
        do {
            // wait for all processes in this process group
            if (p_waitpid(job->pgId, &waitStatus, false) == FAILURE && p_errno != p_ECHILD) {
                return FAILURE;
            }

        } while (!W_WIFEXITED(waitStatus) && !W_WIFSIGNALED(waitStatus) && !W_WIFSTOPPED(waitStatus));

        if (W_WIFSTOPPED(waitStatus)) {
            break;
        }
    }


    if (W_WIFSTOPPED(waitStatus)) {
        job->isRunning = false;
        jobQueuePush(q, job);
        printf("\nStopped: %s\n", job->jobDesc);
    } else {
        freeJob(job);
    }

    return SUCCESS;
}

job **pollJobChanges(jobQueue *q) {

    // check if any of the background processes finished / stopped
    int pollStatus = 0;
    int pollPid = p_waitpid(-1, &pollStatus, true);

    // finished jobs array
    int count = 0;
    job *finishedJobs[jobQueueCount(q)];
    
    if (pollPid == -1 && p_errno != p_ECHILD) {
        return NULL;
    }

    if (pollPid > 0) {
        // some process has finished / stopped, we check for each of our jobs to
        // see if some of their processes have finished / stopped
        job *curr = q->front;
        job *next = NULL;
        while (curr != NULL) {
            next = curr->next;

            for (int i = 0; i < curr->commandCount; i++) {
                int waitStatus = 0;
                int res = p_waitpid(curr->pids[i], &waitStatus, true);

                if (curr->pids[i] != -1 && (pollPid == curr->pids[i] || res == curr->pids[i])) {
                    if (W_WIFSTOPPED(waitStatus) || W_WIFSTOPPED(pollStatus)) {
                        
                        if (curr->isRunning) {
                            printf("\nStopped: %s\n", curr->jobDesc);
                        }
                        // this process was stopped so the entire job has halted
                        curr->isRunning = false;
                    } else if (W_WIFEXITED(waitStatus) || W_WIFSIGNALED(waitStatus) ||
                               W_WIFEXITED(pollStatus) || W_WIFSIGNALED(pollStatus)) {
                        // this process has finished
                        curr->pids[i] = -1;
                        curr->processesFinished = curr->processesFinished + 1;
                    }
                }
            }

            // if we have finished all the processes, remove it from the job jobQueue
            if (curr->processesFinished == curr->commandCount) {
                finishedJobs[count] = jobQueueRemoveJob(q, curr);
                count++;
            }

            // repeat for the next job
            curr = next;
        }
    }

    // create a list of finished jobs on the heap
    job **jobsToSweep = malloc(sizeof(job*) * (count + 1));

    // check for malloc errors
    if (jobsToSweep == NULL) {
        return NULL;
    }

    // copy over finished jobs
    for (int i = 0; i < count; i++) {
        jobsToSweep[i] = finishedJobs[i];
    }

    // null-terminate our array
    jobsToSweep[count] = NULL;

    return jobsToSweep;
}