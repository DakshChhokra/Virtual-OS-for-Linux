#include <ucontext.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "kernel.h"
#include "node.h"
#include "PCB.h"
#include "signal.h"
#include "../include/macros.h"

void setForegroundProcess (pid_t pid) {
	setForeground(pid);
}

int p_nice(pid_t pid, int priority) {

    scheduler *s = getScheduler();
    queue *processTable = getProcessTable();
    node *process = queueSearch(processTable, newNode(pid, NULL));

    if (process == NULL) {
        return -1;
    }

    int prev = process->pcb->priority_level;
    pcb_t *pcb = process->pcb;
    if (prev == -1) {
        process = queueSearch(s->high, process);
    } else if (prev == 0) {
        process = queueSearch(s->med, process);
    } else {
        process = queueSearch(s->low, process);
    }
    removeFromScheduler(process, s);
    pcb->priority_level = priority;
    addToScheduler(newNode(pid, pcb), s);

    fprintf(getLogfile(), "[%d] NICE %d %d %d %s\n", getNumTicks(), pid, prev, priority, pcb->name);

    return 0;
}

pid_t p_spawn(void (*func)(), char*argv[], int fd0, int fd1) {
	FILE *logFile = getLogfile();
	// create child process
	pcb_t *child = k_process_create(getCurrProcess());
	child->stdin = fd0;
	child->stdout = fd1;
	
	child->name = malloc(sizeof(char) * (strlen(argv[0]) + 1));
	strcpy(child->name, argv[0]);

	// check for errors
	if (child == NULL || argv == NULL) {
		return FAILURE;
	}

	// update the context for the child process
	makeContext(&(child->context), func, argv);

	fprintf(logFile, "[%d] CREATED %d %d %s\n", getNumTicks(), child->pid, child->priority_level, child->name);
	return child->pid;
}

pid_t p_waitpid(pid_t pid, int*wstatus, bool nohang) {
	FILE *logFile = getLogfile();

	// check for errors
	if (wstatus == NULL) {
		p_errno = -2;
		return FAILURE;
	}

	queue *processTable = getProcessTable();
	pcb_t *parent = getCurrProcess();


	// check for errors
	if (processTable == NULL || parent == NULL) {
		p_errno = -2;
		return FAILURE;
	}

	if (pid > 0) {
		node *childNode = queueSearch(processTable, newNode(pid, NULL));

		// check for errors
		if (childNode == NULL) {
			p_errno = -1;
			return FAILURE;
		}

		if (!childNode->pcb->waitedOn) {
			fprintf(logFile, "[%d] WAITED %d %d %s\n", getNumTicks(), childNode->pid, childNode->pcb->priority_level, childNode->pcb->name);
			childNode->pcb->waitedOn = true;
		}

		//check if the child is a zombie
		child *currZombie = parent->zombies;
		while (currZombie != NULL) {
			// if child is zombie remove it
			if (currZombie->pid == pid) {
				// remove node from process table
				node *nodeToRemove = queueSearch(processTable, newNode(currZombie->pid, NULL));
				k_process_cleanup(nodeToRemove->pcb);
				clearZombiesAndChildren(nodeToRemove->pcb);
				*wstatus = childNode->pcb->status;
				return pid;
			}
			currZombie = currZombie->next;
		}

		if (nohang) {
			// check if there was a status change
			if (childNode->pcb->prevStatus != -1 && (childNode->pcb->prevStatus != childNode->pcb->status)) {
				childNode->pcb->prevStatus = childNode->pcb->status;
				*wstatus = childNode->pcb->status;

				// unblock parent only if in foreground
				pcb_t *foregroundProcess = getForegroundProcess();
				if (childNode->pid == foregroundProcess->pid) {
					unblockParent(childNode->pcb->ppid);
				}
				return pid;
			}
			return 0;
		} else {
			// check if there was a status change
			if (childNode->pcb->prevStatus != -1 && (childNode->pcb->prevStatus != childNode->pcb->status)) {
				childNode->pcb->prevStatus = childNode->pcb->status;
				*wstatus = childNode->pcb->status;
				
				// unblock parent only if in foreground
				pcb_t *foregroundProcess = getForegroundProcess();
				if (childNode->pid == foregroundProcess->pid) {
					unblockParent(childNode->pcb->ppid);
				}
				return pid;
			}
			childNode->pcb->prevStatus = childNode->pcb->status;
			setForegroundProcess(pid);

			// perform a context switch
			switchContext(0);

			// check if there was a status change
			if (childNode->pcb->prevStatus != childNode->pcb->status) {
				childNode->pcb->prevStatus = childNode->pcb->status;
				*wstatus = childNode->pcb->status;
				
				// unblock parent only if in foreground
				pcb_t *foregroundProcess = getForegroundProcess();
				if (childNode->pid == foregroundProcess->pid) {
					unblockParent(childNode->pcb->ppid);
				}
			}
			return pid;
		}
	} else {
		//check for zombie children
		child *currZombie = parent->zombies;

		if (currZombie != NULL) {
			// remove zombie child
			parent->zombies = parent->zombies->next;
			if (parent->zombies != NULL) {
				parent->zombies->prev = NULL;
			}

			// update wstatus and clean up zombie child
			node *childToRemove = queueSearch(processTable, newNode(currZombie->pid, NULL));

			// check for errors
			if (childToRemove == NULL) {
				p_errno = -1;
				return FAILURE;
			}
			*wstatus = childToRemove->pcb->status;
			k_process_cleanup(childToRemove->pcb);

			// unblock parent only if in foreground
			pcb_t *foregroundProcess = getForegroundProcess();
			if (childToRemove->pid == foregroundProcess->pid) {
				unblockParent(childToRemove->pcb->ppid);
			}
			if (!childToRemove->pcb->waitedOn) {
				fprintf(logFile, "[%d] WAITED %d %d %s\n", getNumTicks(), childToRemove->pid, childToRemove->pcb->priority_level, childToRemove->pcb->name);
				childToRemove->pcb->waitedOn = true;
			}
			return childToRemove->pid;
		}

		// loop through non-zombie children
		child *head = parent->child_pids;

		while (head != NULL) {
			node *childNode = queueSearch(processTable, newNode(head->pid, NULL));
			if (!childNode->pcb->waitedOn) {
				fprintf(logFile, "[%d] WAITED %d %d %s\n", getNumTicks(), childNode->pid, childNode->pcb->priority_level, childNode->pcb->name);
				childNode->pcb->waitedOn = true;
			}
			//check if the status of any child has changed
			if (childNode->pcb->prevStatus != -1 && (childNode->pcb->prevStatus != childNode->pcb->status)) {
				childNode->pcb->prevStatus = childNode->pcb->status;
				*wstatus = childNode->pcb->status;
				pcb_t *foregroundProcess = getForegroundProcess();
				if (childNode->pid == foregroundProcess->pid) {
					unblockParent(childNode->pcb->ppid);
				}
				return childNode->pid;
			}
			// recursively check children and grandchildren
			pid_t res = traverseChild(childNode->pcb, wstatus);
			if (res != 0) {
				return res;
			}
			head = head->next;
		}

		if (nohang) {	
			return 0;
		} else {
			// block parent
            parent->status = BLOCKED;
            fprintf(logFile, "[%d] BLOCKED %d %d %s\n", getNumTicks(), parent->pid, parent->priority_level, parent->name);

			// perform a context switch
			switchContext(0);

			// loop through non-zombie children
			child *head = parent->child_pids;
			while (head != NULL) {
				node *childNode = queueSearch(processTable, newNode(head->pid, NULL));
				if (!childNode->pcb->waitedOn) {
					fprintf(logFile, "[%d] WAITED %d %d %s\n", getNumTicks(), childNode->pid, childNode->pcb->priority_level, childNode->pcb->name);
					childNode->pcb->waitedOn = true;
				}	
				//check if the status of any child has changed
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
				// recursively check children and grandchildren
				pid_t res = traverseChild(childNode->pcb, wstatus);
				if (res != 0) {
					return res;
				}

				head = head->next;
			}

			return 0;
		}
	}
}

int p_kill(pid_t pid, int sig) {
	queue *processTable = getProcessTable();
	node *n = queueSearch(processTable, newNode(pid, NULL));
	
	if (n == NULL) {
		p_errno = -1;
		return -1;
	}

	k_process_kill(n->pcb, sig);
	return 0;
}

void p_exit(void) {
	pcb_t *currProcess = getCurrProcess();

	// check for errors
	if (currProcess == NULL) {
		p_errno = -1;
        return;
	}

	// case when shell needs to exit
	if (currProcess->pid == 1) {
		exit(0);
	}

	// update status
    currProcess->status = EXITED;
    dealWithUnwaitedProcess(currProcess);
    switchContext(0);
}

void checkForTerminalControl() {
	// get currently running process and foreground process
	pcb_t *curr = getCurrProcess();
	pcb_t *foregroundProcess = getForegroundProcess();

	// check if they are the same and if not send a S_SIGSTOP signal
	if (curr->pid != foregroundProcess->pid) {
		if (p_kill(curr->pid, S_SIGSTOP) == -1) {
			p_exit();
		}
	}
	switchContext(0);
}

void p_sleep(unsigned int ticks) {
	// block current process
	pcb_t *currProcess = getCurrProcess();
	if (currProcess == NULL) {
		p_errno = -1;
        p_exit();
    }

	currProcess->status = BLOCKED;
	currProcess->ticksLeft = ticks;
	addToAsleep(newNode(currProcess->pid, currProcess));
}
