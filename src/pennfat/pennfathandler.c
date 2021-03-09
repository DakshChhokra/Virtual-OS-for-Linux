#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#include "../fs/fat.h"
#include "pennfathandler.h"
#include "../fs/file.h"
#include "../include/macros.h"

int handlePennFatCommand(char ***commands, int commandCount, fat **fat) {
    int result = FAILURE;
    char *command = commands[0][0];
    if (strcmp(command, "mkfs") == 0) {
        if (commands[0][1] == NULL || commands[0][2] == NULL || commands[0][3] == NULL) {
            printf("Must supply filename, numBlocks, and blockSizeConfig\n");
            return result;
        }
        result = handleMakeFsCommand(commands[0][1], (char) atoi(commands[0][2]), (char) atoi(commands[0][3]), fat);
    } else if (strcmp(command, "mount") == 0) {
        result = handleMountCommand(commands[0][1], fat);
    } else if (*fat == NULL) {
        printf("No filesystem mounted\n");
        return FAILURE;
    } else if (strcmp(command, "umount") == 0) {
        result = handleUnmountCommand(fat);
    } else if (strcmp(command, "touch") == 0) {
        result = handleTouchCommand(commands[0], *fat);
    } else if (strcmp(command, "mv") == 0) {
        result = handleMoveCommand(commands[0][1], commands[0][2], *fat);
    } else if (strcmp(command, "rm") == 0) {
        result = handleRemoveCommand(commands[0], *fat);
    }  else if (strcmp(command, "cat") == 0) {
        result = handleCatCommand(commands[0], *fat);
    } else if (strcmp(command, "cp") == 0) {
        result = handleCopyCommand(commands[0], *fat);
    } else if (strcmp(command, "ls") == 0) {
        result = handleLsCommand(*fat);
    } else if (strcmp(command, "chmod") == 0) {
        result = handleChmodCommand(commands[0], *fat);
    } else if (strcmp(command, "describe") == 0) {
        printf("Filename  : %s\n", (*fat)->fileName);
        printf("NumBlocks : %d\n", (*fat)->numBlocks);
        printf("BlockSize : %d\n", (*fat)->blockSize);
        printf("NumEntries: %d\n", (*fat)->numEntries);
        printf("FileCount : %d\n", (*fat)->fileCount);
        printf("FreeBlocks: %d\n", (*fat)->freeBlocks);
    } else {
        printf("%s not recognized\n", commands[0][0]);
    }

    return result;
}

int handleMakeFsCommand(char *fileName, uint8_t numBlocks, uint8_t blockSizeIndicator, fat **fat) {
    if (fat != NULL)
        freeFat(fat);

    *fat = getFat(fileName, numBlocks, blockSizeIndicator, true);

    if (*fat == NULL) {
        printf("Failed to make filesystem\n");
        return FAILURE;
    }

    return SUCCESS;
}

int handleMountCommand(char *fileName, fat **fat) {
    if (fileName == NULL) {
        printf("Must supply filename\n");
        return FAILURE;
    }

    if (*fat != NULL)
        freeFat(fat);
    
    *fat = loadFat(fileName);

    if (*fat == NULL)
        return FAILURE;

    return SUCCESS;
}

int handleUnmountCommand(fat **fat) {
    saveFat(*fat);
    freeFat(fat);

    return SUCCESS;
}

int handleTouchCommand(char **files, fat *fat) {
    if (files[1] == NULL) {
        printf("Must supply at least one filename\n");
        return FAILURE;
    }

    int idx = 1;
    char *fileName = files[idx];
    while (fileName != NULL) {
        if (writeFileToFAT(fileName, NULL, 0, 0, REGULAR_FILETYPE, READWRITE_PERMS, fat, true, false, false) == FAILURE)
            return FAILURE;
        fileName = files[++idx];
    }

    saveFat(fat);
    return SUCCESS;
}

int handleMoveCommand(char *oldFileName, char *newFileName, fat *fat) {
    if (oldFileName == NULL || newFileName == NULL) {
        printf("Must supply filename and new filename\n");
        return FAILURE;
    }

    if (renameFile(oldFileName, newFileName, fat) == FAILURE)
        return FAILURE;

    saveFat(fat);
    return SUCCESS;
}

int handleRemoveCommand(char **files, fat *fat) {
    if (files[1] == NULL) {
        printf("Must supply at least one filename\n");
        return FAILURE;
    }

    int idx = 1;
    char *fileName = files[idx];
    while (fileName != NULL) {
        if (deleteFileFromFAT(fileName, fat, false) == FAILURE) {
            return FAILURE;
        }
        fileName = files[++idx];
    }

    saveFat(fat);
    return SUCCESS;
}

int handleCatCommand(char **commands, fat *fat) {
    // count number of arguments and validate input
    // i.e. ensure "-w" or "-a" appears only if second from the end and length >= 2
    int count = 0;
    while (commands[count] != NULL) {
        count++;
    }

    if (count < 2) {
        printf("Must supply at least one argument\n");
        return FAILURE;
    }

    for (int i = 0; i < count; i++) {
        if (strcmp(commands[i], "-w") == 0 && i != count - 2) {
            printf("Invalid -w flag location\n");
            return FAILURE;
        }

        if (strcmp(commands[i], "-a") == 0 && i != count - 2) {
            printf("Invalid -a flag location\n");
            return FAILURE;
        }
    }

    // check if we are writing to a file or appending
    bool writing = strcmp(commands[count - 2], "-w") == 0;
    bool appending = strcmp(commands[count - 2], "-a") == 0;

    // handle redirecting user input to a file
    if (count == 3 && (writing || appending)) {
        // get user input
        char *line = NULL;
        size_t len = 0;
        ssize_t n;

        // reset errno
        RESET_ERRNO

        printf("reading a line\n");
        n = getline(&line, &len, stdin);

        // handle errors in reading the user input
        if (n == -1) {   
            if (line != NULL) {
                free(line);
            }
            // check if failure to get line was due to error
            if (errno == EINVAL || errno == ENOMEM) {
                perror("getline");
                return FAILURE;
            } else {
                ssize_t numBytes = write(STDERR_FILENO, "\n", 1);

                // handle errors for writing
                if (numBytes == -1) {   
                    perror("write");
                    return FAILURE;
                }
                return FAILURE;
            }
        }

        // write line to file, appending if necessary
        printf("writing a file\n");
        if (writeFileToFAT(commands[2], (uint8_t*) line, 0, n, REGULAR_FILETYPE, READWRITE_PERMS, fat, appending, false, false) == FAILURE) {
            free(line);
            return FAILURE;
        }
        free(line);
    } else {
        // concatenate files together and display output to file
        int filesToConcatenate;

        if (writing || appending)
            filesToConcatenate = count - 3;
        else
            filesToConcatenate = count - 1;

        // get all files to concatenate
        file *files[filesToConcatenate];
        for (int i = 0; i < filesToConcatenate; i++) {
            files[i] = readFileFromFAT(commands[i + 1], fat);
            if (files[i] == NULL) {
                for (int j = 0; j < i; j++)
                    freeFile(files[j]);
                return FAILURE;
            }
        }

        printf("writing %d\n", writing);
        printf("appending %d\n", appending);

        // print to stdout or write to file if supplied and free file
        for (int i = 0; i < filesToConcatenate; i++) {

            if (!writing && !appending)
                printf("%s", (char*)files[i]->bytes);
            else if (i == 0 && writing) {
                printf("writing 1\n");
                if (writeFileToFAT(commands[count - 1], files[i]->bytes, 0, files[i]->len, REGULAR_FILETYPE, READWRITE_PERMS, fat, false, false, false) == FAILURE)
                    return FAILURE;
            } else {
                printf("writing 2\n");
                if (writeFileToFAT(commands[count - 1], files[i]->bytes, 0, files[i]->len, REGULAR_FILETYPE, READWRITE_PERMS, fat, true, false, false) == FAILURE)
                    return FAILURE;
            }
            freeFile(files[i]);
        }

        // ensure all output is flushed to stdout
        if (!writing && !appending && fflush(stdout) != 0) {
            perror("fflush");
            return FAILURE;
        }
    }

    if (writing || appending) {
        saveFat(fat);
    }

    return SUCCESS;
}

int handleCopyCommand(char **commands, fat *fat) {
    // count number of arguments
    int count = 0;
    while (commands[count] != NULL) {
        count++;
    }

    if (count < 3) {
        printf("Must supply source file and destination file\n");
        return FAILURE;
    }

    bool copyingFromHost = false;
    bool copyingToHost = false;

    // validate input, i.e. ensure "-h" appears only if second from the end and length = 4
    // or second from the front and length = 4
    for (int i = 0; i < count; i++) {
        if (strcmp(commands[i], "-h") == 0) {
            if (i == 1)
                copyingFromHost = true;
            else if (i == 2) {
                if (copyingFromHost) {
                    printf("Invalid -h flag location\n");
                    return FAILURE;
                }
                copyingToHost = true;
            } else {
                printf("Invalid -h flag location\n");
                return FAILURE;
            }
        }
    }

    // if either copying from host or to host, should have at least four arguments
    if ((copyingFromHost || copyingToHost) && count < 4) {
        printf("Must supply source file and destination file\n");
        return FAILURE;
    }

    if (copyingFromHost) {
        // open the file to read from
        int fd;
        if ((fd = open(commands[2], O_RDONLY, 0644)) == -1) {
            perror("open");
            return FAILURE;
        }

        // get length of the file
        int size = lseek(fd, 0, SEEK_END);
        if (size == -1) {
            printf("Failed to get host file length\n");
            return FAILURE;
        }

        if (lseek(fd, 0, SEEK_SET) == -1) {
            printf("Failed to return to the front of the file after counting size\n");
            return FAILURE;
        }

        // if the file is empty, just create an empty file
        if (size == 0) {
            if (writeFileToFAT(commands[3], NULL, 0, size, REGULAR_FILETYPE, READWRITE_PERMS, fat, false, false, false) == FAILURE) {
                printf("Failed to copy host file %s to %s\n", commands[2], commands[3]);
                return FAILURE;
            }
        }
        // otherwise, read the file into a buffer
        uint8_t *buf = malloc(sizeof(uint8_t) * size);

        // check if malloc failed
        if (buf == NULL) {
            perror("malloc");
            return FAILURE;
        }

        ssize_t bytesRead;
        int bufIdx = 0;

        // read all bytes to buffer
        while ((bytesRead = read(fd, &buf[bufIdx], fminl(size, SSIZE_MAX))) != 0) {
            // check if read failed
            if (bytesRead == -1) {
                perror("read");
                free(buf);
                return FAILURE;
            }
            bufIdx += bytesRead;
            if (bufIdx == size)
                break;
        }

        if (close(fd) == -1) {
            perror("close");
            free(buf);
            return FAILURE;
        }

        // write the file to the FAT
        if (writeFileToFAT(commands[3], buf, 0, size, REGULAR_FILETYPE, READWRITE_PERMS, fat, false, false, false) == FAILURE) {
            printf("Failed to copy host file %s to %s\n", commands[2], commands[3]);
            free(buf);
            return FAILURE;
        }

        free(buf);

        saveFat(fat);
    } else if (copyingToHost) {
        // get entry of file to copy
        file *file = readFileFromFAT(commands[1], fat);

        if (file == NULL)
            return FAILURE;

        // open the file to read from
        int fd;
        if ((fd = open(commands[3], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
            perror("open");
            return FAILURE;
        }

        // write each byte to the file on the host
        for (int i = 0; i < file->len; i++) {
            if (write(fd, &file->bytes[i], sizeof(uint8_t)) == -1) {
                perror("write");
                return FAILURE;
            }
        }

        if (close(fd) == -1) {
            perror("close");
            return FAILURE;
        }

        // free the file
        freeFile(file);
    } else {
        file *file = readFileFromFAT(commands[1], fat);

        if (file == NULL)
            return FAILURE;

        if (writeFileToFAT(commands[2], file->bytes, 0, file->len, REGULAR_FILETYPE, READWRITE_PERMS, fat, false, false, false) == FAILURE) {
            return FAILURE;
        }

        // free the file
        freeFile(file);
        saveFat(fat);
    }

    return SUCCESS;
}

int handleLsCommand(fat *fat) {
    directoryEntryNode *entryNode = fat->firstDirectoryEntryNode;

    while (entryNode != NULL) {
        directoryEntry *entry = entryNode->entry;

        // get permission as printable string
        char *perms;
        switch(entry->perm) {
            case(NONE_PERMS)     : perms = "--"; break;
            case(READ_PERMS)     : perms = "r-"; break;
            case(WRITE_PERMS)    : perms = "-w"; break;
            case(READWRITE_PERMS): perms = "rw"; break;
        }

        // get time in printable strings
        struct tm *localTime = localtime(&entry->mtime);
        
        // buffers for month, day, time strings
        char month[4];
        char day[3];
        char time[6];

        // fill buffers with relevant data
        strftime(month, 4, "%b", localTime);
        strftime(day, 3, "%d", localTime);
        strftime(time, 6, "%H:%M", localTime);


        // print entry
        printf("%2s%6db%4s%3s%6s %s\n", perms, entry->size, month, day, time, entry->name);

        entryNode = entryNode->next;
    }

    return SUCCESS;
}

int handleChmodCommand(char **commands, fat *fat) {
    if (commands[1] == NULL) {
        printf("Must supply a filename\n");
        return FAILURE;
    }

    if (commands[2] == NULL) {
        printf("Must supply permission type\n");
        return FAILURE;
    }

    int perm = 0;

    if (strcmp(commands[2], "-w") == 0) {
        perm = WRITE_PERMS;
    } else if (strcmp(commands[2], "r-") == 0) {
        perm = READ_PERMS;
    } else if (strcmp(commands[2], "rw") == 0) {
        perm = READWRITE_PERMS;
    } else if (strcmp(commands[2], "--") == 0) {
        perm = NONE_PERMS;
    } else {
        printf("Permission type must be one of -w, r-, rw, and --\n");
    }

    if (chmodFile(fat, commands[1], perm) == FAILURE)
        return FAILURE;

    saveFat(fat);
    return SUCCESS;
}