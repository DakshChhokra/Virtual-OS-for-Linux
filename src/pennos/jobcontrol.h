#include <stdbool.h>
#include "jobQueue.h"

/**
 * @file jobcontrol.h
 * @brief Contains logic for facilitating job control in the shell
 */

/**
 * Takes in an array of strings from parsejob and returns true
 * if the command is a job control command
 * @param  command    the array of strings from parsejob
 * @return            true only if the commands represent a job control command
 */
bool isJobControlCommand(char **command);

/**
 * Only called when isJobControlCommand returns true and passes the
 * command to the correct handler
 * @param  command     the array of strings from parsejob
 * @param  q    the job jobQueue to handle commands on
 * @param  currentPgId a pointer to the currentPgId in the parent
 */
void handleJobControlCommand(char **command, jobQueue *q, int *currentPgId);


/**
 * Runs the chosen job in the job jobQueue in the foreground
 * @param  q    the job jobQueue to handle commands on
 * @param  job         the job to resume in the foreground if a valid command, or NULL
 * @param  currentPgId a pointer to the currentPgId in the parent
 * @return             0 on success, or -1 on failure
 */
int handleForegroundCommand(jobQueue *q, job *job, int *currentPgId);

/**
 * Runs the chosen stopped job in the job jobQueue in the background
 * @param  q the job jobQueue to handle commands on
 * @param  job      the job to resume in the background if a valid command, or NULL
 * @return          0 on success, or -1 on failure
 */
int handleBackgroundCommand(jobQueue *q, job *job);

/**
 * Lists the jobs in the job jobQueue
 * @param  q   the job jobQueue to handle commands on
 * @return            0 on success, or -1 on failure
 */
int handleJobsCommand(jobQueue *q);

/**
 * Puts a given job in the foreground
 * @param  q    the job jobQueue to handle commands on
 * @param  job         the job to put in the foreground
 * @param  interactive whether the shell is in interactive mode
 * @return             0 on success, or -1 on failure
 */
int putJobInForeground(jobQueue *q, job *job, bool interactive);

/**
 * Polls for any state changes and if so, looks for completed jobs
 * and returns an array of those jobs
 * @param  q The jobQueue to search for jobs from
 * @return          a null-terminated array of finished job pointers, or NULL on failure
 */
job **pollJobChanges(jobQueue *q);