#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unistd.h>

#include "queue.h"
#include "shell.h"
#include "node.h"
#include "PCB.h"
#include "kernel.h"
#include "../include/macros.h"
#include "filedescriptor.h"

void signalhandler(int signum) {
    pcb_t *currProcess = getForegroundProcess();
    write(STDERR_FILENO, "\n", 1);
    if (currProcess->pid != 1) {
        if (signum == SIGINT) {
            p_kill(currProcess->pid, S_SIGTERM);
        } else if (signum == SIGTSTP) {
            p_kill(currProcess->pid, S_SIGSTOP);
        }
    } else {
        queue *processTable = getProcessTable();
        queuePrint(processTable);

        // prompt the user and get their inputs
        write(STDERR_FILENO, PENNOS_PROMPT, PENNOS_PROMPT_LENGTH);
    }
}

// the signal interrupt context
ucontext_t signal_context;

// file for logging
FILE *logFile;

// scheduer
scheduler *s;

// a pointer to the current process selected by the scheduler
node *currProcess = NULL;

// a pointer to the foreground process node
node *foregroundProcess = NULL;

// the scheduler context
ucontext_t schedulerContext;

// main's context
ucontext_t mainContext;

// the idle process context and parameter
ucontext_t idleContext;
bool inIdle = false;

// processTable queue
queue *processTable = NULL;

// a queue for all the sleep processes
queue *asleep = NULL;

bool timeExpired = false;

// cariable for the total number of ticks that have passed
int numTicks = 0;

FILE *getLogfile() {
    return logFile;
}

pcb_t *getForegroundProcess() {
    return foregroundProcess->pcb;
}

pcb_t *getCurrProcess() {
    return currProcess->pcb;
}

queue *getProcessTable() {
    return processTable;
}

scheduler *getScheduler() {
    return s;
}

int getNumTicks() {
    return numTicks;
}

/*
 * Helper function which sets the stack for a context appropriately
 * @param stack, pointer to stack_t struct to contain the new stack
 */
void setStack(stack_t *stack) {
    void *sp = malloc(SIGSTKSZ);
    *stack = (stack_t) { .ss_sp = sp, .ss_size = SIGSTKSZ };
}

void makeContext(ucontext_t *ucp,  void (*func)(), char *argv[]) {
    // get current context
    getcontext(ucp);
    sigemptyset(&ucp->uc_sigmask);
    setStack(&ucp->uc_stack);
    ucp->uc_link = &schedulerContext;

    // associate it with the input function
    makecontext(ucp, func, 1, argv);
}

/*
 * Helper function for adding pid's (in the form of nodes) 
 * to a childrent or zombie list
 * @param pid, the pid of the new child to be added
 * @param queue, the queue where the pid needs to be added (in the form of a node)
 */
child *addChild(int pid, child *queue) {
    // create a new child struct
    child *c = malloc(sizeof(child));
    c->prev = NULL;
    c->pid = pid;

    // insert it to the queue
    if (queue == NULL) {
        c->next = NULL;
    } else {
        c->next = queue;
        queue->prev = c;
    }

    return c;
}

pcb_t *k_process_create(pcb_t *parent) {
    // create the new process
    pcb_t *process = malloc(sizeof(pcb_t));


    if (process == NULL) {
        p_errno = -3;
        perror("malloc");
        return NULL;
    }

    process->ppid = parent->pid;
    process->pgid = parent->pgid;
    process->context = parent->context;
    process->status = READY;
    process->prevStatus = READY;
    process->priority_level = 0;
    process->waitedOn = false;
    // generate new pid
    if (processTable->count == 0) {
        process->pid = 1;
    } else {
        process->pid = processTable->back->pid + 1;
    }
    process->ticksLeft = -1;
    process->child_pids = NULL;
    process->zombies = NULL;
    process->stdin = STDIN_FILENO;
    process->stdout = STDOUT_FILENO;

    // add the current process to the children list of the parent
    parent->child_pids = addChild(process->pid, parent->child_pids);

    // create a node for the new process and add it to the process 
    // table and scheduler queue
    node *n1 = newNode(process->pid, process);
    queuePush(processTable, n1);
    node *n2 = newNode(process->pid, process);
    addToScheduler(n2, s);
    return process;
}

void k_process_cleanup(pcb_t *process) {
    // remove the process from the scheduler queue and process table
    node *toRemove = queueSearch(processTable, newNode(process->pid, NULL));
    queueRemoveNode(processTable, toRemove);
    if (toRemove == NULL) {
        p_errno = -1;
        return;
    }
    removeFromScheduler(toRemove, s);
    node *parent = queueSearch(processTable, newNode(process->ppid, NULL));
    if (parent != NULL) {
        child *currChild = parent->pcb->child_pids;
        // remove process from parent's children list
        while (currChild != NULL) {
            if (currChild->pid == toRemove->pid) {
                if (currChild->prev != NULL) {
                    currChild->prev->next = currChild->next;
                } else {
                    parent->pcb->child_pids = currChild->next;
                }

                if (currChild->next != NULL) {
                    currChild->next->prev = currChild->prev;
                }
                break;
            }
            currChild = currChild->next;
        }

        // remove process from parent's zombie list
        child *currZombie = parent->pcb->zombies;
        while (currZombie != NULL) {
            if (currZombie->pid == toRemove->pid) {
                if (currZombie->prev != NULL) {
                    currZombie->prev->next = currZombie->next;
                } else {
                    parent->pcb->zombies = currZombie->next;
                }

                if (currZombie->next != NULL) {
                    currZombie->next->prev = currZombie->prev;
                }
                break;
            }
            currZombie = currZombie->next;
        }
    }
}


pid_t traverseChild(pcb_t *process, int *wstatus) {
    
    //check for zombies
    child *zombieChild = process->zombies;
    if (zombieChild != NULL) {
        // get rid of zombie
        node *childNode = queueSearch(processTable, newNode(zombieChild->pid, NULL));
        childNode->pcb->prevStatus = childNode->pcb->status;
        *wstatus = childNode->pcb->status;
        k_process_cleanup(childNode->pcb);
        return zombieChild->pid;
    }

    // iterate through children
    child *child = process->child_pids;
    while (child != NULL) {
        node *childNode = queueSearch(processTable, newNode(child->pid, NULL));
        
        // check if there was a status change
        if (childNode->pcb->prevStatus != -1 && (childNode->pcb->prevStatus != childNode->pcb->status)) {
            childNode->pcb->prevStatus = childNode->pcb->status;
            *wstatus = childNode->pcb->status;
            
            // unblock parent only if in foreground
            pcb_t *foregroundProcess = getForegroundProcess();
            if (childNode->pid == foregroundProcess->pid) {
                unblockParent(childNode->pcb->ppid);
            }
            return childNode->pid;
        }

        // recursively check grandchildren
        pid_t res = traverseChild(childNode->pcb, wstatus);
        if (res != 0) {
            return res;
        }
        child = child->next;
    }
    return 0;
}

void clearZombiesAndChildren(pcb_t *process) {
    //clean up zombies of process
    child *zombieChild = process->zombies;
    while (zombieChild != NULL) {
        node *nodeToRemove = queueSearch(processTable, newNode(zombieChild->pid, NULL));
        if (nodeToRemove != NULL) {
            fprintf(logFile, "[%d] ORPHAN %d %d %s\n", numTicks, nodeToRemove->pcb->pid, nodeToRemove->pcb->priority_level, nodeToRemove->pcb->name);
            k_process_cleanup(nodeToRemove->pcb);
            clearZombiesAndChildren(nodeToRemove->pcb);
        }
        zombieChild = zombieChild->next;
    }

    //clean up children of current process
    child *children = process->child_pids;
    while (children != NULL) {
        node *nodeToRemove = queueSearch(processTable, newNode(children->pid, NULL));
        if (nodeToRemove != NULL) {
            fprintf(logFile, "[%d] ORPHAN %d %d %s\n", numTicks, nodeToRemove->pcb->pid, nodeToRemove->pcb->priority_level, nodeToRemove->pcb->name);
            k_process_cleanup(nodeToRemove->pcb);
            clearZombiesAndChildren(nodeToRemove->pcb);
        }
        children = children->next;
    }
}

/*
 * Function for unblocking a parent process, used when a child that has 
 * been waited on changes state
 * @param ppid, the parent pid
 */
void unblockParent(pid_t ppid) {
    node *parent = queueSearch(processTable, newNode(ppid, NULL));

    if (parent == NULL) {
        p_errno = -1;
        return;
    }

    if (parent->pcb->status == BLOCKED) {
        fprintf(logFile, "[%d] UNBLOCKED %d %d %s\n", numTicks, parent->pcb->pid, parent->pcb->priority_level, parent->pcb->name);
        parent->pcb->status = READY;
        setForeground(ppid);
    }
}

void dealWithUnwaitedProcess(pcb_t *process) {
    // remove from scheduler queue
    removeFromScheduler(newNode(process->pid, process), s);

    // add child to parent's zombie queue
    node *parent = queueSearch(processTable, newNode(process->ppid, NULL));
    parent->pcb->zombies = addChild(process->pid, parent->pcb->zombies);

    if (!process->waitedOn) {
        fprintf(logFile, "[%d] ZOMBIE %d %d %s\n", numTicks, process->pid, process->priority_level, process->name);
    }

    // clean the processes zombies and children
    clearZombiesAndChildren(process);
}

void k_process_kill(pcb_t *process, int signal) {
    if (signal == S_SIGSTOP) {
        // update process status
        process->status = STOPPED;
        if (process->pid == foregroundProcess->pid) {
            unblockParent(process->ppid);
        }
        fprintf(logFile, "[%d] STOPPED %d %d %s\n", numTicks, process->pid, process->priority_level, process->name);
        switchContext(0);
    } else if (signal == S_SIGCONT) {
        // update process status
        if (process->status == STOPPED) {
            if (strcmp(process->name, "sleep") == 0) {
                process->status = BLOCKED;
            } else {
                process->status = READY;
            }
            fprintf(logFile, "[%d] CONTINUED %d %d %s\n", numTicks, process->pid, process->priority_level, process->name);
        }
        switchContext(0);
    } else {
        // update status
        process->status = SIGNALED;
        if (process->pid == foregroundProcess->pid) {
            unblockParent(process->ppid);
        }
        dealWithUnwaitedProcess(process);
        fprintf(logFile, "[%d] SIGNALED %d %d %s\n", numTicks, process->pid, process->priority_level, process->name);
        switchContext(0);
    }
}

/*
 * Function for reducing the number of ticks by one for all 
 * the asleep processes
 */
void decrementTicks() {
    node *head = asleep->front;
    while (head != NULL) {
        // if there are more than one ticks left decrease them 
        // by one if the process is still running
        if (head->pcb->ticksLeft > 0) {
            if (head->pcb->status != STOPPED) {
                head->pcb->ticksLeft -= 1;
            }
        } else {
            // deal with finished sleep childs only once
            if (head->pcb->ticksLeft == 0 && head->pcb->status != EXITED) {
                if (head->pcb->status != SIGNALED) {
                    head->pcb->status = EXITED;
                }
                fprintf(logFile, "[%d] EXITED %d %d %s\n", numTicks, head->pcb->pid, head->pcb->priority_level, head->pcb->name);
                node *parent = queueSearch(processTable, newNode(head->pcb->ppid, NULL));
                dealWithUnwaitedProcess(head->pcb);
                queueRemoveNode(asleep, head);
                if (head->pid == foregroundProcess ->pid) {
                    unblockParent(parent->pid);
                }
            }
        }
        head = head->next;
    }
}

/*
 * Function for idle process
 */
void idle() {
    sigset_t mask;
    sigemptyset(&mask);
    sigsuspend(&mask);
}

void schedule() {
    decrementTicks();
    // handle processes that terminate on their own
    if (!timeExpired && currProcess->pcb->ticksLeft != -2) {
        if (currProcess->pcb->ticksLeft <= 0) {
            currProcess->pcb->status = EXITED;
            fprintf(logFile, "[%d] EXITED %d %d %s\n", numTicks, currProcess->pcb->pid, currProcess->pcb->priority_level, currProcess->pcb->name);
            dealWithUnwaitedProcess(currProcess->pcb);
            if (currProcess->pid == foregroundProcess->pid) {
                unblockParent(currProcess->pcb->ppid);
            }
        }
        switchContext(0);
    }

    // get the next process
    currProcess = getNextProcess(s);

    // if no processes are available run idle process
    if (currProcess == NULL) {
        inIdle = true;
        setcontext(&idleContext);
    }

    inIdle = false;
    timeExpired = false;
    // log schedule event
    pid_t pid = currProcess->pid;
    int queue = currProcess->pcb->priority_level;

    fprintf(logFile, "[%d] SCHEDULE %d %d %s\n", numTicks, pid, queue, currProcess->pcb->name);

    // update foreground process
    timeExpired = false;

    // switch context to the next process
    setcontext(&(currProcess->pcb->context));
}


void switchContext(int signum) {
    timeExpired = true;

    // increment tick count only when signum = SIGALRM
    if (signum == SIGALRM) {
        numTicks += 1;
    }
    if (inIdle) {
        setcontext(&schedulerContext);
    } else {
        swapcontext(&(currProcess->pcb->context), &schedulerContext);
    }
}

/*
 * Sets the signal alarm handler for SIGALRM
 */
void setAlarmHandler(void) {
    struct sigaction act;

    act.sa_handler = switchContext;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);

    sigaction(SIGALRM, &act, NULL);
}

/*
 * Creates the itimerval object which will generate the clock ticks
 */
void setTimer(void) {
    struct itimerval it;

    it.it_interval = (struct timeval) { .tv_usec = QUANTUM * 1000};
    it.it_value = it.it_interval;

    setitimer(ITIMER_REAL, &it, NULL);
}

void addToAsleep(node *n) {
    queuePush(asleep, n);
}

/*
 * Function for initializing the the necessary variables 
 * and structs for the kernel
 */
void startKernel(void) {
    // open file for logging
    logFile = fopen(LOGFILE, "w");
    if (logFile == NULL) {
        perror("log");
        exit(EXIT_FAILURE);
    }

    // create process table, asleep queue, and scheduler queue
    processTable = queueInit();
    asleep = queueInit();
    s = schedulerInit();

    // create scheduler context
    char *argv[2] = {"schedule", NULL};
    makeContext(&schedulerContext, schedule, argv);

    // create idle context
    inIdle = false;
    char *argv1[2] = {"idle", NULL};
    makeContext(&idleContext, idle, argv1);

    // set alarm handler and timer
    setAlarmHandler();
    setTimer();
}

void setForeground (pid_t pid) {
    node *n = queueSearch(processTable, newNode(pid, NULL));
    if (n != NULL) {
        foregroundProcess = n;
        if (n->pid != 1) {
            node *parent = queueSearch(processTable, newNode(n->pcb->ppid, NULL));
            // block parent
            parent->pcb->status = BLOCKED;
            fprintf(logFile, "[%d] BLOCKED %d %d %s\n", numTicks, parent->pid, parent->pcb->priority_level, parent->pcb->name);
        }
    }
}


int main(int argc, char *argv[]) {

    // relay S_SIGTERM to foreground process
    if (signal(SIGINT, signalhandler) == SIG_ERR) {   
        perror("signal");
        exit(EXIT_FAILURE);
    }

    // relay S_SIGSTOP to foreground process
    if (signal(SIGTSTP, signalhandler) == SIG_ERR) {   
        perror("signal");
        exit(EXIT_FAILURE);
    }

    // mount file system
    if (argc < 2) {
        printf("Must supply file system to mount\n");
        exit(FAILURE);
    } else
        mountedFat = loadFat(argv[1]);

    if (mountedFat == NULL) {
        if (argc < 4) {
            printf("Must supply number of blocks and block size indicator\n");
            exit(FAILURE);
        } else
            mountedFat = getFat(argv[1], atoi(argv[2]), atoi(argv[3]), true);
    }

    if (mountedFat == NULL) {
        printf("Failed to mount or create file system\n");
        exit(FAILURE);
    }

    container = newContainer();
    if (container == NULL) {
        printf("Failed to create file descriptor container\n");
        exit(FAILURE);
    }

    startKernel();

    // initialize the shell process
    pcb_t *process = malloc(sizeof(pcb_t));
    process->ppid = 1;
    process->pgid = 1;
    process->status = READY;
    process->prevStatus = READY;
    process->priority_level = -1;
    process->pid = 1;
    process->ticksLeft = -1;
    process->child_pids = NULL;
    process->zombies = NULL;
    process->waitedOn = false;
    process->name = "shell";
    process->stdin = STDIN_FILENO;
    process->stdout = STDOUT_FILENO;

    // set the process' context to run the shell
    char *shellArgs[2] = {"shell", NULL};
    makeContext(&(process->context), shell, shellArgs);

    // add shell process to process table and scheduler queue
    node *n1 = newNode(process->pid, process);
    queuePush(processTable, n1);
    node *n2 = newNode(process->pid, process);
    addToScheduler(n2, s);

    fprintf(logFile, "[%d] CREATED %d %d %s\n", numTicks, process->pid, process->priority_level, process->name);

    // initialize the current process and start running it
    queue *processTable = getProcessTable();
    foregroundProcess = processTable->front;
    currProcess = processTable->front;
    setcontext(&(currProcess->pcb->context));
}
