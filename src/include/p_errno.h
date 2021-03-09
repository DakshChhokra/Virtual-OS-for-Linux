/*
 * File for defining the error handling in PennOS
 */
#ifndef P_ERROR_H
#define P_ERROR_H

int p_errno;

#define p_ECHILD -1
#define p_EINVAL -2
#define p_ENOMEM -3
/*
 * Function that prints an error message based on the value of p_errno
 * @param message, a user inputted message
 */
void p_perror(char *message);

#endif