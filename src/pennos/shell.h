#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include "../include/parsejob.h"
#include "user_level_funcs.h"
#include "signal.h"
/**
 * @file shell.h
 * @brief Contains shell built-ins and the shell loop
 */

/*
 * @brief Function for running the kernel shell
 */
void shell();

/*
 * Function for simulating an infinite busy wait
 */
void busy();

/**
 * @brief Function for printing first ten lines of specified files to console
 * 
 * @param argv list of files to call head on
 */
void head(char **argv);

/*
 * Function for the ps command
 */
void ps();

/*
 * Function for sending signal to process
 */
void killer(char *argv[]);

/*
 * Function for creating a zombie process
 */
void zombify();

/*
 * Function for creating an orphan process
 */
void orphanify();

/*
 * Function for printing all the available commands
 */
void man();

/*
 * Wrapper function for running sleep
 */
void createSleep(char **argv);

/**
 * @brief      Lists all the files in the mounted filesystem
 */
void ls();

/**
 * @brief      Creates empty files if they do not exist or update timestamp otherwise
 *
 * @param      argv  The arguments to parse
 */
void touch(char **argv);

/**
 * @brief      Rename src to dest
 *
 * @param      argv  The arguments to parse
 */
void mv(char **argv);

/**
 * @brief      Copy src to a new file dest
 *
 * @param      argv  The arguments to parse
 */
void cp(char **argv);

/**
 * @brief      Removes files
 *
 * @param      argv  The arguments to parse
 */
void rm(char **argv);

/**
 * @brief      Concatenates files to a file descriptor or reads input and spits it back to user
 *
 * @param      argv  The arguments array
 */
void cat(char **argv);

/**
 * @brief      Changes the permission of a file
 *
 * @param      argv  The arguments array
 */
void chmod(char **argv);

#endif