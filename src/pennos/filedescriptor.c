#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#include "filedescriptor.h"
#include "kernel.h"
#include "../fs/file.h"
#include "../include/macros.h"
#include "user_level_funcs.h"

fdContainer *newContainer() {
    fdContainer *out = malloc(sizeof(fdContainer));

    if (out == NULL) {
        perror("malloc");
        return NULL;
    }

    out->firstFdNode = NULL;
    out->lastFdNode = NULL;

    return out;
}

/**
 * @brief      Creates and adds a new file descriptor with specified filename and node and assigns an id
 *
 * @param      fileName   The file name for the descriptor
 * @param[in]  mode       The mode for the descriptor
 *
 * @return     Pointer to the new file descriptor node, or NULL if it failed
 */
fdNode *newFileDescriptorNode(char *fileName, int mode, directoryEntry *entry) {
    fdNode *newNode = malloc(sizeof(fdNode));

    if (newNode == NULL) {
        perror("malloc");
        return NULL;
    }

    newNode->entry = entry;

    newNode->id = 3;
    newNode->mode = mode;
    if (mode == F_APPEND)
        newNode->pos = entry->size;
    else
        newNode->pos = 0;
    newNode->next = NULL;

    if (container->firstFdNode == NULL) {
        container->firstFdNode = newNode;
        container->lastFdNode = newNode;
    } else {
        newNode->id = container->lastFdNode->id + 1;
        container->lastFdNode->next = newNode;
        container->lastFdNode = newNode;
    }

    return newNode;
}

/**
 * @brief      Finds a file descriptor node with the given filename, if it exists
 *
 * @param      fileName   The file name
 *
 * @return     Pointer to the file descriptor node, or NULL if it doesn't exist
 */
fdNode *findFdNodeWithFileName(char *fileName) {
    fdNode *found = container->firstFdNode;

    while (found != NULL && found->entry != NULL) {
        if (strcmp(fileName, found->entry->name) == 0)
            return found;
        found = found->next;
    }

    return found;
}

/**
 * @brief      Finds a file descriptor node with the given filename in either F_WRITE or F_APPEND, if it exists
 *
 * @param      fileName  The file name
 *
 * @return     Pointer to the file descriptor node, or NULL if it doesn't exist
 */
fdNode *findWritingFdNodeWithFileName(char *fileName) {
    fdNode *found = container->firstFdNode;

    while (found != NULL) {
        if ((found->mode == F_WRITE || found->mode == F_APPEND) && found->entry != NULL && strcmp(fileName, found->entry->name) == 0)
            return found;
        found = found->next;
    }

    return found;

}

/**
 * @brief      Finds a file descriptor node with the given filename and the given mode, if it exists
 *
 * @param      fileName   The file name
 * @param[in]  mode       The mode
 *
 * @return     Pointer to the file descriptor node, or NULL if it doesn't exist
 */
fdNode *findFdNodeWithFileNameAndMode(char *fileName, int mode) {
    fdNode *found = container->firstFdNode;

    while (found != NULL) {
        if (found->mode == mode && found->entry != NULL && strcmp(fileName, found->entry->name) == 0)
            return found;
        found = found->next;
    }

    return found;
}

/**
 * @brief      Finds the file descriptor with the given id, if it exists
 *
 * @param[in]  id         The identifier
 *
 * @return     Pointer to the file descriptor node, or NULL if it doesn't exist
 */
fdNode *findFdNodeWithId(int id) {
    fdNode *found = container->firstFdNode;

    while (found != NULL) {
        if (found->id == id)
            return found;
        found = found->next;
    }

    return found;
}

int f_open(char* fname, int mode) {
    if (mode == F_WRITE || mode == F_APPEND) {
        if (findWritingFdNodeWithFileName(fname) != NULL) {
            printf("%s already open for writing\n", fname);
            return FAILURE;
        }

        directoryEntryNode *entryNode;
        getEntryNodeAndPrev(NULL, &entryNode, fname, mountedFat);

        // create the file if it doesn't exist
        if (entryNode == NULL) {
            if (writeFileToFAT(fname, NULL, 0, 0, REGULAR_FILETYPE, READWRITE_PERMS, mountedFat, false, false, false) == FAILURE) {
                printf("Failed to create %s\n", fname);
                return FAILURE;
            }
            getEntryNodeAndPrev(NULL, &entryNode, fname, mountedFat);
        } else if (F_WRITE) {
            // truncate the file if we are in F_WRITE mode
            if (writeFileToFAT(fname, NULL, 0, 0, entryNode->entry->type,entryNode->entry->perm, mountedFat, false, false, false) == FAILURE) {
                printf("Failed to truncate %s\n", fname);
                return FAILURE;
            };
        }

        fdNode *newFd = newFileDescriptorNode(fname, mode, entryNode->entry);

        if (newFd == NULL) {
            printf("Failed to open fd for %s\n", fname);
        }

        return newFd->id;
    } else if (mode == F_READ) {
        directoryEntryNode *entryNode;
        getEntryNodeAndPrev(NULL,  &entryNode, fname, mountedFat);

        if (entryNode == NULL) {
            printf("%s not found", fname);
            return FAILURE;
        }

        fdNode *newFd = newFileDescriptorNode(fname, mode, entryNode->entry);

        if (newFd == NULL) {
            printf("Failed to open fd for %s\n", fname);
        }

        return newFd->id;
    } else {
        printf("Invalid mode\n");
        return FAILURE;
    }
}

int f_read(int fd, int n, uint8_t *buf) {

    if (fd == STDIN_FILENO) {
        checkForTerminalControl();
        int bytesRead = 0;
        if ((bytesRead = read(fd, buf, n)) == -1) {
            printf("Failed to read from stdin\n");
            return FAILURE;
        }

        return bytesRead;
    }
    else {    
        fdNode *node = findFdNodeWithId(fd);
        if (node == NULL) {
            printf("File descriptor %d not found\n", fd);
            return FAILURE; 
        }

        file *f = readFileFromFAT(node->entry->name, mountedFat);
        if (f == NULL) {
            printf("Failed to read file\n");
            return FAILURE;
        }

        if (node->pos >= f->len) {
            // if EOF, return 0
            return 0;
        }

        // otherwise, read at most n bytes
        int bytesToRead = n;

        if (node->pos + n > f->len) {
            bytesToRead = f->len - node->pos;
        }

        memcpy(buf, &(f->bytes[node->pos]), bytesToRead);

        node->pos += bytesToRead;

        freeFile(f);

        return bytesToRead;
    }
}

int f_write(int fd, uint8_t *buf, int n) {
    
    if (fd == STDOUT_FILENO) {
        int totalBytesWritten = 0;
        int bytesWritten = 0;
        while (totalBytesWritten < n && (bytesWritten = write(fd, &buf[totalBytesWritten], n - totalBytesWritten)) != -1) {
            totalBytesWritten += bytesWritten;
        }
        return SUCCESS;
    } else {
        fdNode *node = findFdNodeWithId(fd);
        if (node == NULL) {
            printf("File descriptor %d not found\n", fd);
            return FAILURE; 
        }

        if (node->mode == F_READ) {
            printf("Cannot write in read-only mode\n");
            return FAILURE;
        }

        if (node->mode == F_WRITE) {
            file *f = readFileFromFAT(node->entry->name, mountedFat);

            if (f == NULL)
                return FAILURE;

            if (writeFileToFAT(node->entry->name, buf, node->pos, n, f->type, f->perm, mountedFat, false, false, false) == FAILURE)
                return FAILURE;

            node->pos += n;
        } else if (node->mode == F_APPEND) {
            if (appendToFileInFAT(node->entry->name, buf, n, mountedFat, false) == FAILURE)
                return FAILURE;

            node->pos = node->entry->size;
        }
        return SUCCESS;
    }


    
}

int f_close(int fd) {
    // get fd and the previous node

    fdNode *prev = NULL;
    fdNode *found = container->firstFdNode;

    while (found != NULL) {
        if (found->id == fd)
            break;
        prev = found;
        found = found->next;
    }

    if (found == NULL) {
        printf("File descriptor %d not found\n", fd);
        return FAILURE;
    }

    if (found == container->firstFdNode && found == container->lastFdNode) {
        container->firstFdNode = NULL;
        container->lastFdNode = NULL;
    } else if (found == container->firstFdNode) {
        container->firstFdNode = found->next;
    } else if (found == container->lastFdNode) {
        container->lastFdNode = prev;
    } else {
        prev->next = found->next;
    }

    free(found);

    saveFat(mountedFat);

    return SUCCESS;
}

int f_mv(char *src, char *dest) {
    if (src == NULL || dest == NULL) {
        printf("Must supply filename and new filename\n");
        return FAILURE;
    }

    if (renameFile(src, dest, mountedFat) == FAILURE)
        return FAILURE;


    saveFat(mountedFat);

    return SUCCESS;
}

int f_unlink(char *fileName) {
    if (deleteFileFromFAT(fileName, mountedFat, false) == FAILURE) {
        return FAILURE;
    }

    saveFat(mountedFat);

    return SUCCESS;
}

int f_lseek(int fd, int offset, int whence) {
    if (whence != F_SEEK_CUR && whence != F_SEEK_END && whence != F_SEEK_SET) {
        printf("Invalid whence argument\n");
        return FAILURE;
    }

    fdNode *node = findFdNodeWithId(fd);

    if (node == NULL) {
        printf("File descriptor %d not found\n", fd);
    }

    if (whence == F_SEEK_CUR) {
        if (node->pos + offset > node->entry->size || node->pos + offset < 0) {
            printf("Cannot lseek behind 0 or past EOF\n");
            return FAILURE;
        }
        node->pos += offset;
    } else if (whence == F_SEEK_SET) {
        if (offset < 0 || offset > node->entry->size) {
            printf("Cannot lseek behind 0 or past EOF\n");
            return FAILURE;
        }
        node->pos = offset;
    } else if (whence == F_SEEK_END) {
        if (offset > 0 || (offset < 0 && abs(offset) > node->entry->size)) {
            printf("Cannot lseek behind 0 or past EOF\n");
            return FAILURE;
        }
        node->pos = node->entry->size + offset;
    }

    return node->pos;
}

void f_ls() {
    directoryEntryNode *entryNode = mountedFat->firstDirectoryEntryNode;

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
        char str[1024];
        sprintf(str, "%2s%6db%4s%3s%6s %s\n", perms, entry->size, month, day, time, entry->name);

        pcb_t *currProcess = getCurrProcess();
        f_write(currProcess->stdout, (uint8_t *) str, strlen(str));

        entryNode = entryNode->next;
    }
}

int f_chmod(char *fileName, int permission) {
    if (chmodFile(mountedFat, fileName, permission) == FAILURE)
        return FAILURE;

    saveFat(mountedFat);
    
    return SUCCESS;
}