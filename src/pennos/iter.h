#include <stdbool.h>
#include "jobQueue.h"

/**
 * @file iter.h
 * @brief Contains the core logic of a shell loop iteration
 */

/**
 * @brief Processes user input into the shell
 * @param line the user input string
 * @param interactive whether this is interactive mode or not
 * @param currentPgId a pointer to currentPgId in the parent
 * @param q    a pointer to the job queue
 */
void iter (char *line, bool interactive, int *currentPgId, jobQueue *q);

/**
 * @brief Frees the passed parameters and then exits with EXIT_FAILURE
 * @param jobCommands the parsed commands
 * @param jobStdin    the redirected stdin, or null
 * @param jobStdout   the redirected stdout, or null
 * @param queue       the job queue to destroy
 * @param line        the buffer line used by getline in main
 * @param errMsg      the message to display with perror
 */
void exitGracefully(char ***jobCommands, char *jobStdin, char *jobStdout, jobQueue *queue, char *line, char *errMsg);