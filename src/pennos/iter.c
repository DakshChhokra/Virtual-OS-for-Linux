#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "iter.h"
#include "../include/macros.h"
#include "../include/parsejob.h"
#include "handlejob.h"
#include "jobcontrol.h"
#include "filedescriptor.h"
#include "user_level_funcs.h"
#include "shell.h"


/**
 * @brief Frees the passed parameters and then exits with EXIT_FAILURE
 * @param jobCommands the parsed commands
 * @param jobStdin    the redirected stdin, or null
 * @param jobStdout   the redirected stdout, or null
 * @param queue       the job queue to destroy
 * @param line        the buffer line used by getline in main
 * @param errMsg      the message to display with perror
 */
void exitGracefully(char ***jobCommands, char *jobStdin, char *jobStdout, jobQueue *queue, char *line, char *errMsg) {
    freeJobCommands(jobCommands);
    if (jobStdin != NULL)
        free(jobStdin);
    if (jobStdout != NULL)
        free(jobStdout);
    if (line != NULL) 
        free(line);
    jobQueueDestroy(queue);
    p_exit();
}

void iter(char *line, bool interactive, int *currentPgId, jobQueue* jobjobQueue) {
    // check if line is a valid command
    if (parseJob(line, interactive)) {
        // get job details
        char ***jobCommands = getJobCommands();
        int commandCount = getCommandCount();
        bool isBackground = isBackgroundJob();

        // TODO: Handle redirections

        char *jobStdin = getJobStdin();
        char *jobStdout = getJobStdout();
        bool isAppending = isAppendingStdout();



        // file descriptor numbers for stdin/stdout
        int infile = STDIN_FILENO;
        int outfile = STDOUT_FILENO;

        //initialize garbage
        int file_in = -1;
        int file_out = -1;

        // redirect stdin if necessary
        if (jobStdin != NULL) {
            file_in = f_open(jobStdin, F_READ);
            // handle error case for open
            if (file_in == FAILURE) {
                exitGracefully(jobCommands, jobStdin, jobStdout, jobjobQueue, line, "open-in");
            } else {
                //big dup2 energy
                infile = file_in;
            }
            

            
        }



        // redirect stdout if necessary
        if (jobStdout != NULL) {
            if (isAppending) {
                file_out = f_open(jobStdout, F_APPEND);
            } else {
                file_out = f_open(jobStdout, F_WRITE);
            }

            // handle error case for open
            if (file_out == FAILURE) {
                exitGracefully(jobCommands, jobStdin, jobStdout, jobjobQueue, line, "open-out");
            } else {
                //big dup2 energy
                outfile = file_out;
            }
        }

        // check if the first command is subroutine command
        if (isJobControlCommand(jobCommands[0])) {
            handleJobControlCommand(jobCommands[0], jobjobQueue, currentPgId);
        } else if (strcmp(jobCommands[0][0], "man") == 0) {
            man();
        } else if (strcmp(jobCommands[0][0], "logout") == 0) {
            p_exit();
        } else if (strcmp(jobCommands[0][0], "nice_pid") == 0) {
            int priority = atoi(jobCommands[0][1]);
            pid_t pid = atoi(jobCommands[0][2]);
            p_nice(pid, priority);
        } else {
            // otherwise create a new job
            job *thisJob = newJob(jobCommands, commandCount, infile, outfile);

            // check if we succesfully created a new job
            if (thisJob == NULL) {
                exitGracefully(jobCommands, jobStdin, jobStdout, jobjobQueue, line, "newJob");
            }

            // run job by iterate through and running each command in parallel
            for (int i = 0; i < commandCount; i++) {
                int childpid = handleJob(jobCommands, commandCount, i, thisJob, currentPgId);

                if (childpid == 0) {
                    if (jobStdin != NULL)
                        free(jobStdin);
                    if (jobStdout != NULL)
                        free(jobStdout);
                    if (line != NULL)
                        free(line);
                    freeJobCommands(jobCommands);
                    return;
                }

                // see if the execution succeeded
                if (childpid == FAILURE) {
                    freeJob(thisJob);
                    exitGracefully(jobCommands, jobStdin, jobStdout, jobjobQueue, line, "handlejob");
                }

                // set current PGID to be the PGID for this job (first child's PID)
                if (i == 0) {
                    *currentPgId = thisJob->pgId;
                }
            }

            // finished business logic for jobs at this point
            // print message for background jobs
            if (isBackground) {
                // add job to jobQueue
                if (jobQueuePush(jobjobQueue, thisJob) == NULL) {
                    exitGracefully(jobCommands, jobStdin, jobStdout, jobjobQueue, line, "jobQueuePush");
                }
                printRunningJob(thisJob);
            } else {
                if (putJobInForeground(jobjobQueue, thisJob, interactive) == FAILURE) {
                    freeJob(thisJob);
                    exitGracefully(jobCommands, jobStdin, jobStdout, jobjobQueue, line, "putJobInForeground");
                }
            }
        }

        // close the outfile if it isn't stdout
        if (outfile != STDOUT_FILENO && f_close(outfile) == -1) {
            exitGracefully(jobCommands, jobStdin, jobStdout, jobjobQueue, line, "close");
        }

        // close the infile if it isn't stdin
        if (infile != STDIN_FILENO && f_close(infile) == -1) {
            exitGracefully(jobCommands, jobStdin, jobStdout, jobjobQueue, line, "close");
        }

        // free allocated memory for this job
        if (jobStdin != NULL)
            free(jobStdin);
        if (jobStdout != NULL)
            free(jobStdout);
        if (line != NULL)
            free(line);
        freeJobCommands(jobCommands);
        RESET_ERRNO
    }
}
