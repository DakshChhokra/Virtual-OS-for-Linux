/*
        Author: Hannah Pan

        Date:   09/23/2020
*/

/**
 * @file parsejob.h
 * @brief Library for parsing inputs, provided by the instructors
 */


#ifndef PARSEJOB_H
#define PARSEJOB_H

#include <stdbool.h>  // bool, false, true

bool parseJob(char *cmdLine, bool tty);

char *getJobStdin(void);
char *getJobStdout(void);
char ***getJobCommands(void);
int getCommandCount(void);
bool isAppendingStdout(void);
bool isBackgroundJob(void);

void freeJobCommands(char **commands[]);

void printJob(void);  // for demo use only

#endif


/*

DESCRIPTION

    The parseJob() function parses a pipeline of commands stored in cmdLine. If
    penn-shell is running as an interactive shell, tty should be set to true.
    Otherwise, it should be set to false.

    The getJobStdin() and getJobStdout() functions retrieve the file names of
    redirected stdin and stdout, respectively. The caller of these functions is
    responsible for freeing the memory allocated for the file name.

    The getJobCommands() function retrieves the array (NULL-terminated) of
    parsed commands, each of which can be passed to execvp(3) as its second
    argument. The caller of this function is responsible for freeing the memory
    allocated for the array by calling freeJobCommands().

    The freeJobCommands() function frees the memory allocated to the commands of
    the parsed job.

    The printJob() function prints the most recently parsed job to stdout. (It
    is for demo use only.)

RETURN VALUE

    The parseJob() function returns true if the job in cmdLine is valid, or
    false if it is invalid or cmdLine is empty.

    The getJobStdin() and getJobStdout() functions return a pointer to the file
    name of redirected stdin or stdout if present, or NULL otherwise.

    The getJobCommands() function returns a pointer to the array of the parsed
    commands if cmdLine is valid, or NULL otherwise.

    The getCommandCount() function returns the number of commands in cmdLine.

    The isAppendingStdout() function returns true if the job has appeding
    redirected stdout, or false otherwise.

    The isBackgroundJob() functions returns true if the job is a background job,
    or false otherwise.

*/
