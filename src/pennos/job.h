#include <stdbool.h>

#ifndef JOB_HEADER
#define JOB_HEADER

/**
 * @file job.h
 * @brief Contains a job abstraction for the shell
 */

/**
 * A job with pointers to next/prev jobs to allow a linked-list of jobs
 *
 * Each job has a jobId, a pgId (initially -1), a job description, and its input/output file descriptors.
 * Every job is also responsible for mallocing and freeing an array of pids (for each command) and
 * mallocing, closing, and freeing the array of pipes that it uses in its pipeline.
 *
 * Jobs are passed to handleJob() [commandCount] number of times, in sequence, and pgId is initialized the
 * first time it is passed. Each handleJob() call lazily creates pipes and assigns pids to each command in
 * the job.
 */
typedef struct jobTag {
    int jobId;
    int pgId;

    // user inputted command
    char* jobDesc;
    // number of commands in the pipeline
    int commandCount;
    // number of processes that have succesfully been waited
    int processesFinished;

    // pointer to the array of pids for each command in the pipeline
    int *pids;
    // pipes for this job
    int (*pipes)[2];
    // input file descriptor number for this job
    int infile;
    // output file descriptor number for this job
    int outfile;

    // whether the job is running or stopped
    bool isRunning;

    // linked list prev pointer
    struct jobTag* prev;

    // linked list next pointer
    struct jobTag* next;
} job;

/**
 * @brief Initialize a new job, with no process group id until first passed into handleJob()
 * @param  jobCommands  the array of commands from parsejob
 * @param  commandCount the number of commands in this job
 * @param  infile       the input file descriptor number for this job
 * @param  outfile      the output file descriptor number for this job
 * @return              a pointer to the job, or NULL if it failed to initialize
 */
job *newJob(char ***jobCommands, int commandCount, int infile, int outfile);

/**
 * @brief Frees a job and its job description
 * @param j a job pointer to free
 */
void freeJob(job *j);

/**
 * @brief Prints a job in a human-readable format
 * @param j       a job pointer to print
 */
void printJobDetails(job *j);

/**
 * @brief Prints the "Running: <command>" message for background jobs
 * @param  j a job pointer to print the message for
 */
void printRunningJob(job *j);

/**
 * @brief Prints the "Finished: <command>" message for background jobs
 * @param  j a job pointer to print the message for
 */
void printFinishedJob(job *j);

#endif