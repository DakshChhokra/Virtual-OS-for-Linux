/*
        Author: Kyven Wu

        Date:   10/29/20
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "pennfat.h"
#include "../include/macros.h"
#include "../include/parsejob.h"
#include "pennfathandler.h"

void exitGracefully(int exitVal, fat *fat) {
    if (fat != NULL)
        freeFat(&fat);
    exit(exitVal);
}

void signalHandler(int sigNum) {
    if (sigNum == SIGINT) {
        // prompt the user and get their inputs
        ssize_t numBytes = write(STDERR_FILENO, "\n", 1);
        // handle errors for writing
        if (numBytes == -1) {
            perror("write");
            exitGracefully(EXIT_FAILURE, NULL);
        }

        numBytes = write(STDERR_FILENO, PENNFAT_PROMPT, PENNFAT_PROMPT_LENGTH);
        // handle errors for writing
        if (numBytes == -1) {
            perror("write");
            exitGracefully(EXIT_FAILURE, NULL);
        }
    }
}

int main(void) {
    // initialize currFat to NULL
    fat *currFat = NULL;

    // bind signal handler for sigint
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        perror("signal");
        exitGracefully(FAILURE, currFat);
    }
    // interactive mode
    while (1) {
        RESET_ERRNO

        // prompt the user and get their inputs
        ssize_t numBytes = write(STDERR_FILENO, PENNFAT_PROMPT, PENNFAT_PROMPT_LENGTH);

        // handler errors for writing
        if (numBytes == -1) {
            perror("write");
            exitGracefully(EXIT_FAILURE, currFat);
        }


        char *line = NULL;
        size_t len = 0;
        ssize_t n;

        // reset errno
        RESET_ERRNO

        n = getline(&line, &len, stdin);

        // handle errors in reading the user input
        if (n == -1) {   
            if (line != NULL) {
                free(line);
            }
            // check if failure to get line was due to error
            if (errno == EINVAL || errno == ENOMEM) {
                perror("getline");
                exitGracefully(EXIT_FAILURE, currFat);
            } else {
                numBytes = write(STDERR_FILENO, "\n", 1);

                // handle errors for writing
                if (numBytes == -1) {   
                    perror("write");
                    exitGracefully(EXIT_FAILURE, currFat);
                }
                exitGracefully(EXIT_SUCCESS, currFat);
            }
        }

        // handle cases where the user presses only enter
        if (n == 1 && line[n - 1] == '\n') {
            // free the buffer
            if (line != NULL) {
                free(line);
            }
        } else {
            if (parseJob(line, true)) {
                char ***jobCommands = getJobCommands();
                handlePennFatCommand(jobCommands, getCommandCount(), &currFat);
                freeJobCommands(jobCommands);
            }
            free(line);
        }
    }
}