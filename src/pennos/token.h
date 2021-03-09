#ifndef TOKEN_H
#define TOKEN_H

/**
 * @file token.h
 * @brief Contains a function reads the parsed job commands and concatenates them to form a string
 */

/**
 * Returns a pointer to a string representing the job
 * @param  jobCommands  a pointer to the array of commands for the job
 * @param  commandCount the number of commands in this job
 * @return              a pointer to the string representing the job
 */
char *getCommandStringFromTokens(char ***jobCommands, int commandCount);

char ***getCopyOfCommands(char ***jobCommands, int commandCount);

#endif