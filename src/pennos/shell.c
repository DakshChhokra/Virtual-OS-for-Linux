#include <stdio.h>
#include <unistd.h>

#include "kernel.h"
#include "shell.h"
#include "job.h"
#include "handlejob.h"  
#include "jobcontrol.h"
#include "iter.h"
#include "filedescriptor.h"
#include "../include/macros.h"
#include <stdlib.h>

char *commands[] = {
    "zombify", 
    "orphanify", 
    "man", 
    "bg [job_id]", 
    "fg [job_id]", 
    "jobs", 
    "logout", 
    "cat", 
    "sleep n", 
    "busy", 
    "ls", 
    "touch file ...", 
    "mv src dest", 
    "cp src dest", 
    "rm file ...", 
    "ps", 
    "kill -[SIGNAL_NAME] pid ...", 
    "nice_pid priority pid",
    "nice priority command [arg]"};

void busy() {
    while(1) {
        // busy wait indefinitely
    }
}

void head(char **argv) {
    /*
    printf("entered\n");
    
    printf("%s\n", argv[count]);
    int fd = f_open(argv[count], F_READ); 
    printf("fd is %d\n", fd);
    uint8_t buffer[4096];
    int bytes_read = 0;
    bytes_read =  f_read(fd, 4096, buffer);
    printf("br is %d\n", bytes_read);
    int bytes_written = 0;
    bytes_written = f_write(getCurrProcess()->stdout, buffer, bytes_read);
    printf("bw is %d\n", bytes_written);
    int fc = f_close(fd);
    printf("fc is %d\n", fc);
    */
    int count = 1;
    while (argv[count] != NULL) {
        //printf("%s\n", argv[count]);
        int tb = 4 + 4 + strlen(argv[count]) + 1 +1;
        char title_buffer[tb];
        sprintf(title_buffer, "\n==> %s <==\n", argv[count]);
        
        f_write(getCurrProcess()->stdout, (uint8_t *) title_buffer, tb);
        
        int fd = f_open(argv[count], F_READ); 
        //printf("fd is %d\n", fd);       
        
        uint8_t *big_buffer = malloc(sizeof(char) * 40960);
        


        int bytes_read = 0;
        bytes_read =  f_read(fd, 40960, big_buffer);
        //printf("br is %d\n", bytes_read);

        

        if (bytes_read == FAILURE) {
            printf("Failed to read from file.\n");
                
        } else {
            int eol_counter = 0;
            int last_num = 0;
            for(int i = 0; i < bytes_read; i++) {
                char check  = (char) big_buffer[i];
                if (check == '\n') {
                    eol_counter++;
                }
                if (eol_counter == 10) {
                    last_num = i;
                    break;
                }
            }
            uint8_t *buffer_after_count = malloc(sizeof(char) * last_num);
            
            for (int i = 0; i < last_num; i++) {
                buffer_after_count[i] = big_buffer[i];
            }


            int bytes_written = 0;
            bytes_written = f_write(getCurrProcess()->stdout, buffer_after_count, last_num);
            //printf("bw is %d\n", bytes_written);
            free(buffer_after_count);
        }
        f_close(fd);
        //printf("fc is %d\n", fc);
        count++;
        
        free(big_buffer);
        
    }
    

}

void createSleep(char **argv) {
    if (argv[1] == NULL) {
        // indefinite sleep case
        p_sleep(-2);
    } else {
        p_sleep(atoi(argv[1]) * 10);
    }

}

void killer(char *argv[]) {
    // get pid of process to send signal tos
    pid_t pid = atoi(argv[2]);

    // send S_SIGTERM to process
    if (strcmp(argv[1], "-term") == 0) {
        p_kill(pid, S_SIGTERM);
    // send S_SIGCONT to process
    } else if (strcmp(argv[1], "-cont") == 0) {
        p_kill(pid, S_SIGCONT);
    // send S_SIGSTOP to process
    } else {
        p_kill(pid, S_SIGSTOP);
    }
}

void man() {
    size_t len = sizeof(commands)/sizeof(commands[0]);
    int i;
    for (i = 0; i < len; i++) {
        printf("%s\n", commands[i]);
    }
}

void ps() {
    scheduler *s = getScheduler();
    
    char columns[20];
    sprintf(columns, "PID PPID PRIORITY\n");
    write(STDERR_FILENO, columns, strlen(columns));

    queue *high = s->high;
    node *curr = queueFront(high);
    while (curr != NULL) {
        if (curr->pcb->status == READY) {

            // build output string with pid, ppid, and priority
            char pid[20];
            sprintf(pid, "%d ", curr->pcb->pid);

            char ppid[20];
            sprintf(ppid, "%d ", curr->pcb->ppid);

            char priority[20];
            sprintf(priority, "%d\n", curr->pcb->priority_level);

            strcat(ppid, priority);
            strcat(pid, ppid);
            int length = strlen(pid);

            // write to shell
            write(STDERR_FILENO, pid, length);
        }
        curr = curr->next;
    }

    queue *med = s->med;
    curr = queueFront(med);
    while (curr != NULL) {
        if (curr->pcb->status == READY) {

            // build output string with pid, ppid, and priority
            char pid[20];
            sprintf(pid, "%d ", curr->pcb->pid);

            char ppid[20];
            sprintf(ppid, "%d ", curr->pcb->ppid);

            char priority[20];
            sprintf(priority, "%d\n", curr->pcb->priority_level);

            strcat(ppid, priority);
            strcat(pid, ppid);
            int length = strlen(pid);

            // write to shell
            write(STDERR_FILENO, pid, length);
        }
        curr = curr->next;
    }

    queue *low = s->low;
    curr = queueFront(low);
    while (curr != NULL) {
        if (curr->pcb->status == READY) {

            /// build output string with pid, ppid, and priority
            char pid[20];
            sprintf(pid, "%d ", curr->pcb->pid);

            char ppid[20];
            sprintf(ppid, "%d ", curr->pcb->ppid);

            char priority[20];
            sprintf(priority, "%d\n", curr->pcb->priority_level);

            strcat(ppid, priority);
            strcat(pid, ppid);
            int length = strlen(pid);

            // write to shell
            write(STDERR_FILENO, pid, length);
        }
        curr = curr->next;
    }
}

void zombie_child() {
    // MMMMM Brains...!
    return;
}

void zombify() {
    char *argv[2] = {"zombie_child", NULL};
    p_spawn(zombie_child, argv, STDIN_FILENO, STDOUT_FILENO);
    while (1);
    return;
}

void orphan_child() {
    // Please sir,
    // I want some more
    while (1) ;
}

void orphanify() {
    char *argv[2] = {"orphan_child", NULL};
    p_spawn(orphan_child, argv, STDIN_FILENO, STDOUT_FILENO);
    return;
}

void cat(char **argv) {

    if (argv[1] == NULL) {
        int bufSize = 4096;
        uint8_t buf[bufSize];

        int bytesRead = 0;

        while ((bytesRead = f_read(getCurrProcess()->stdin, bufSize, buf)) != 0) {
            if (bytesRead == -1) {
                printf("Failed to read from stdin\n");
                return;
            }

            if (f_write(getCurrProcess()->stdout, buf, bytesRead) == FAILURE)
                return;
        }

        return;
    }

    int idx = 1;
    while (argv[idx] != NULL) {
        // open the file in read mode
        int fd = f_open(argv[idx], F_READ);
        // check if f_open failed
        if (fd == FAILURE)
            return;

        int bufSize = 64;
        uint8_t buf[bufSize];

        int bytesRead = 0;
        while ((bytesRead = f_read(fd, bufSize, buf)) != 0) {
            if (bytesRead == -1) {
                printf("Failed to read from fd %d\n", fd);
                return;
            }

            // write the buffer to STDOUT
            if (f_write(getCurrProcess()->stdout, buf, bytesRead) == FAILURE)
                return;
        }

        if (f_close(fd) == FAILURE)
            return;

        idx++;
    }
}

void ls() {
    f_ls();
}

void list_fds() {
    fdNode *node = container->firstFdNode;

    while (node != NULL) {
        printf("File %s at fdno %d in mode %d at pos %d\n", node->entry->name, node->id, node->mode, node->pos);
        node = node->next;
    }
}

void touch(char **argv) {
    int idx = 1;
    while (argv[idx] != NULL) {
        // open the file in append mode
        int fd = f_open(argv[idx], F_APPEND);
        // check if f_open failed
        if (fd == FAILURE)
            return;

        // touch the file
        if (f_write(fd, NULL, 0) == FAILURE)
            return;

        if (f_close(fd) == FAILURE)
            return;

        idx++;
    }
}

void mv(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        printf("Must provide src and dest files\n");
        return;
    }
    
    f_mv(argv[1], argv[2]);
}

/**
 * @brief      Copy src to a new file dest
 *
 * @param      argv  The arguments to parse
 */
void cp(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        printf("Must provide src and dest files\n");
        return;
    }
    char *src = argv[1];
    char *dest = argv[2];
    int src_fd = f_open(src, F_READ);

    if (src_fd == FAILURE)
        return;

    int size = f_lseek(src_fd, 0, SEEK_END);

    if (size == FAILURE)
        return;

    // get bytes of src file
    uint8_t *buf = malloc(sizeof(uint8_t) * size);

    if (buf == NULL) {
        perror("malloc");
        f_close(src_fd);
        return;
    }

    int bytesRead = 0;
    while ((bytesRead = f_read(src_fd, size, &(buf[bytesRead]))) != 0) {
        if (bytesRead == FAILURE) {
            f_close(src_fd);
            free(buf);
            return;
        }
    }

    if (f_close(src_fd) == FAILURE) {
        free(buf);
        return;
    }


    int dest_fd = f_open(dest, F_WRITE);

    if (dest_fd == FAILURE)
        return;

    if (f_write(dest_fd, buf, size) == FAILURE) {
        f_close(dest_fd);
        free(buf);
        return;
    }

    free(buf);

    f_close(dest_fd);
}

void rm(char **argv) {
    int idx = 1;
    while (argv[idx] != NULL) {
        // remove the file
        if (f_unlink(argv[idx]) == FAILURE)
            return;

        idx++;
    }
}

void chmod(char **argv) {
    if (argv[1] == NULL) {
        printf("Must supply a filename\n");
        return;
    }

    if (argv[2] == NULL) {
        printf("Must supply permission type\n");
        return;
    }

    int perm = 0;

    if (strcmp(argv[2], "r-") == 0) {
        perm = READ_PERMS;
    } else if (strcmp(argv[2], "-w") == 0) {
        perm = WRITE_PERMS;
    } else if (strcmp(argv[2], "rw") == 0) {
        perm = READWRITE_PERMS;
    } else if (strcmp(argv[2], "--") == 0) {
        perm = NONE_PERMS;
    } else {
        printf("Permission type must be one of w-, -r, rw, and --\n");
    }

    f_chmod(argv[1], perm);
}

void destroyQueueAndExit(jobQueue *jobQueue, int exitVal) {
    jobQueueDestroy(jobQueue);
    p_exit();
}

pid_t currentPgId = -1;

void shell() {

    // initialize job queue
    jobQueue *jobQueue = jobQueueInit();

    // ignore SIGTTOU signals
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
        destroyQueueAndExit(jobQueue, EXIT_FAILURE);
    }

	while (1) {
        RESET_ERRNO
        currentPgId = -1;

        queue *processTable = getProcessTable();
        queuePrint(processTable);

        // prompt the user and get their inputs
        ssize_t numBytes = write(STDERR_FILENO, PENNOS_PROMPT, PENNOS_PROMPT_LENGTH);

        // handler errors for writing
        if (numBytes == -1) {
            destroyQueueAndExit(jobQueue, EXIT_FAILURE);
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t n = 0;

         // reset errno
        RESET_ERRNO

        checkForTerminalControl();
        n = getline(&line, &len, stdin);
        // handle errors in reading the user input
        if (n == -1) {   
            if (line != NULL) {
                free(line);
            }
            // check if failure to get line was due to error
            if (p_errno == p_EINVAL || p_errno == p_ENOMEM) {
                destroyQueueAndExit(jobQueue, EXIT_FAILURE);
            } else {
                numBytes = f_write(STDOUT_FILENO, (uint8_t *) "\n", 1);

                // handle errors for writing
                if (numBytes == -1) {
                    destroyQueueAndExit(jobQueue, EXIT_FAILURE);
                }
                destroyQueueAndExit(jobQueue, EXIT_SUCCESS);
            }

        }
        // poll for jobs after user input to ensure we are up to date


        job **finishedJobs = pollJobChanges(jobQueue);

        // handle cases where the user presses only enter
        if (n == 1 && line[n - 1] == '\n') {
            // free the buffer
            if (line != NULL) {
                free(line);
            }
        } else {
        	iter(line, true, &currentPgId, jobQueue);
        }

        int index = 0;
        while (finishedJobs != NULL && finishedJobs[index] != NULL) {
            printFinishedJob(finishedJobs[index]);
            freeJob(finishedJobs[index]);
            index++;
        }

        // free finished jobs array if not null
        if (finishedJobs != NULL) {
            free(finishedJobs);
        }

        fflush(getLogfile());
    }
}