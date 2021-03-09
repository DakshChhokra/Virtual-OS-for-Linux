/**
 * @file user_level_funcs.h
 * @brief Contains user level functions for dealing with processes
 */

/*
 * User level function for forking a new thread that retains most of the attributes of the parent thread. 
 * Once the thread is spawned, it executes the function referenced by func with its argument array argv. 
 * @param func pointer to function to be run by the new child process
 * @param argv array of parameters for func
 * @return the pid of the child thread on success, or -1 on error
 */
pid_t p_spawn(void (*func)(), char*argv[], int fd0, int fd1);

/*
 * User level function for setting the calling thread as blocked (ifnohangis false) until a child of the
 * calling thread changes state. If nohang is true, p_waitpid does not block but returns immediately.
 * @param pid the pid of the child to wait on
 * @param wstatus a pointer to be updated to the stats of the child
 * @param nohang whether this is a blocking wait or not
 * @return the pid of the child which has changed state on success, or -1 on error.
 */
pid_t p_waitpid(pid_t pid, int*wstatus, bool nohang);

/*
 * User level function for sending the signal sig to the thread referenced by pid.
 * @param pid the pid of the process to send the signal to
 * @param sig the signal code
 * @return 0 on success, or -1 on error.
 */
int p_kill(pid_t pid, int sig);

/*
 * User level function for setting the thread pid to blocked until ticks of the 
 * system clock elapse, and then sets the thread to running. Importantly, p_sleep 
 * should not return until the thread resumes running; however, it can be 
 * interrupted by a S_SIGTERM signal.
 */
void p_sleep(unsigned int ticks);


/* 
 * User level function for exitting the current thread unconditionally.
 */
void p_exit(void);

/*
 * Function for setting the foreground process
 * @param pid, the pid of the process to set in the foreground
 */
void setForegroundProcess (pid_t pid);

/*
 * Function for checking if current process has terminal control
 */
void checkForTerminalControl();

/**
 * @brief      Sets priority of process
 *
 * @param      pid       Pid of process whose priority will be set
 * @param      priority  priority to be set
 *
 * @return     0 if successful else -1
 */
int p_nice(pid_t pid, int priority);