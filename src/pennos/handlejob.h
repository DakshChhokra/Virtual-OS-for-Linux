#include <stdbool.h>

#include "job.h"

/**
 * @file handlejob.h
 * @brief Contains a handler to handle jobs inputted into the shell
 */


/**
 * @brief      Handles the terminal control signal
 *
 * @param[in]  signum  The signum
 */
void terCtrlSighandler(int signum);

/**
 * [handleJob description]
 * @param  commands     the array (NULL-terminated) of parsed commands
 * @param  commandCount the number of commands in the parsed input
 * @param  index        the index of the current command of the job
 * @param  job          this job pointer for this job
 * @param  currentPgId  a pointer to the currentPgId in the parent
 * @return              currentPgId if job completed successfully, -1 otherwise
 */
int handleJob(
    char ***commands,
    int commandCount,
    int index,
    job *job,
    int *currentPgId
);
