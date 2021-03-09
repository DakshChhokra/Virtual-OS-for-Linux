#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#include "fat.h"
#include "file.h"
#include "../include/macros.h"

directoryEntryNode *newDirectoryEntryNode(
    char *fileName,
    uint32_t size,
    uint16_t firstBlock,
    uint8_t type,
    uint8_t perm,
    time_t time
) {
    directoryEntryNode *outputNode = malloc(sizeof(directoryEntryNode));
    outputNode->entry = malloc(sizeof(directoryEntry));
    outputNode->next = NULL;
    directoryEntry *entry = outputNode->entry;
    entry->size = size;
    entry->firstBlock = firstBlock;
    entry->type = type;
    entry->perm = perm;
    entry->mtime = time;

    for (int i = 0; i < 32; i++)
        entry->name[i] = '\0';
    
    strcpy(entry->name, fileName);

    for (int i = 0; i < 16; i++)
        entry->reserved[i] = '\0';

    return outputNode;
}

void freeDirectoryEntryNode(directoryEntryNode *node) {
    free(node->entry);
    free(node);
}

fat *getFat(char *fileName, uint8_t numBlocks, uint8_t blockSizeIndicator, bool creating) {
    // check if numBlocks is valid
    if (numBlocks < 1 || numBlocks > 32) {
        printf("Number of blocks must be between 1 and 32\n");
        return NULL;
    }

    // check if numBlocks is valid
    if (blockSizeIndicator < 1 || blockSizeIndicator > 4) {
        printf("Block size config must be between 1 and 4\n");
        return NULL;
    }

    // allocate space for this FAT
    fat *output = malloc(sizeof(fat));

    // check if malloc failed
    if (output == NULL) {
        perror("malloc");
        return NULL;
    }

    // set FAT filename on disk
    int len = strlen(fileName);
    output->fileName = malloc(len * sizeof(uint8_t) + 1);
    for (int i = 0; i < len + 1; i++)
        output->fileName[i] = '\0';
    strcpy(output->fileName, fileName);

    // set number of blocks in the FAT to numBlocks
    output->numBlocks = numBlocks;

    // set blocksize depending on input
    switch (blockSizeIndicator) {
        case 1:  output->blockSize = 512; break;
        case 2:  output->blockSize = 1024; break;
        case 3:  output->blockSize = 2048; break;
        case 4:  output->blockSize = 4096; break;
        default: output->blockSize = 1024; break;
    }

    // set numEntries to be blockSize * numBlocks / 2
    output->numEntries = (output->blockSize * output->numBlocks) / 2;

    // initialize linked list to be empty and number of files to zero
    output->fileCount = 0;
    output->firstDirectoryEntryNode = NULL;
    output->lastDirectoryEntryNode = NULL;

    // set free space equal to total space
    output->freeBlocks = output->numEntries - 2;

    // open the file to write to and check for errors
    int fd;
    if (creating) {
        // if creating / overwriting, truncate file
        if ((fd = open(fileName, O_RDWR | O_TRUNC | O_CREAT, 0644)) == -1) {
        perror("open");
        return NULL;
        }

        // truncate file to fatSize bytes and check for errors
        if (ftruncate(fd, output->numBlocks * output->blockSize) == -1) {
            perror("ftruncate");
            return NULL;
        }
    } else {
        // otherwise, just load the file
        if ((fd = open(fileName, O_RDWR, 0644)) == -1) {
        perror("open");
        return NULL;
        }
    }
    
    // map FAT table in memory to disk
    output->blocks = mmap(NULL, output->numBlocks * output->blockSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // check if mmap failed
    if (output->blocks == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    // close fd
    if (close(fd) == -1) {
        perror("close");
        return NULL;
    }

    // set first block to store FAT metadata
    output->blocks[0] = (uint16_t) numBlocks << 8 | blockSizeIndicator;

    // end of file links in the first block for root directory if first time initializing
    if (output->blocks[1] == 0x0000)
        output->blocks[1] = 0xFFFF;

    return output;
}

/**
 * @brief      Given a directory file entry
 *
 * @param[out] fat                 The FAT filesystem
 * 
 * @return     SUCCESS on success, FAILURE on any failed malloc
 */
int loadDirectoryEntries(fat *fat) {
    // check if the FAT already has directory entries initialized
    if (fat->fileCount != 0)
        return FAILURE;

    // get file for the directory entry
    file *directoryFile = getDirectoryFile(fat);

    if (directoryFile == NULL) {
        // Directory file not initialized
        return SUCCESS;
    }
    
    // read every file entry after the directory file entry and append to linked list
    for (int i = 0; i < directoryFile->len; i = i + 64) {
        // allocate memory for new entry node
        directoryEntryNode *newNode = malloc(sizeof(directoryEntryNode));
        newNode->next = NULL;

        if (newNode == NULL) {
            perror("malloc");
            return FAILURE;
        }

        // allocate memory for new entry
        directoryEntry *newEntry = malloc(sizeof(directoryEntry));

        if (newEntry == NULL) {
            free(newNode);
            perror("malloc");
            return FAILURE;
        }

        // copy bytes over to initialize directory entry
        memcpy((uint8_t *) &newEntry->name, &directoryFile->bytes[i], 32 * sizeof(uint8_t));
        memcpy((uint8_t *) &newEntry->size, &directoryFile->bytes[i + 32], 4 * sizeof(uint8_t));
        memcpy((uint8_t *) &newEntry->firstBlock, &directoryFile->bytes[i + 36], 2 * sizeof(uint8_t));
        memcpy((uint8_t *) &newEntry->type, &directoryFile->bytes[i + 38], 1 * sizeof(uint8_t));
        memcpy((uint8_t *) &newEntry->perm, &directoryFile->bytes[i + 39], 1 * sizeof(uint8_t));
        memcpy((uint8_t *) &newEntry->mtime, &directoryFile->bytes[i + 40], 8 * sizeof(uint8_t));
        memcpy((uint8_t *) &newEntry->reserved, &directoryFile->bytes[i + 48], 16 * sizeof(uint8_t));

        // set newNode's entry to be newEntry
        newNode->entry = newEntry;

        // check if list is empty
        if (fat->fileCount == 0) {
            // initialize list
            fat->firstDirectoryEntryNode = newNode;
            fat->lastDirectoryEntryNode = newNode;
        } else {
            // add new entry node to the end of the linked list
            fat->lastDirectoryEntryNode->next = newNode;
            fat->lastDirectoryEntryNode = newNode;
        }

        // reduce the number of freeblocks by 
        fat->freeBlocks -= ceil((double) newEntry->size / fat->blockSize);

        // increment fileCount
        fat->fileCount++;
    }

    // free the directory file
    freeFile(directoryFile);

    return SUCCESS;
}

fat *loadFat(char *fileName) {
    // open the file to read from and check for errors
    int fd;
    if ((fd = open(fileName, O_RDONLY, 0644)) == -1) {
        perror("open");
        return NULL;
    }

    // read the next byte as indicator of block size and set accordingly
    // note we read this first even though the 2 bytes we write to disk are in
    // a different order because our OS is little-endian.
    uint8_t blockSizeIndicator = 0;
    if (read(fd, &blockSizeIndicator, sizeof(uint8_t)) == -1) {
        perror("read");
        return NULL;
    }

    // read the next byte as number of blocks
    uint8_t numBlocks = 0;
    if (read(fd, &numBlocks, sizeof(uint8_t)) == -1) {
        perror("read");
        return NULL;
    }

    // close fd
    if (close(fd) == -1) {
        perror("close");
        return NULL;
    }

    // get this FAT
    fat *output = getFat(fileName, numBlocks, blockSizeIndicator, false);

    if (output == NULL) {
        printf("Failed to load FAT\n");
        return NULL;
    }

    // load directory entries into directory entry linked list
    if (loadDirectoryEntries(output) == FAILURE) {
        freeFat(&output);
        return NULL;
    }

    return output;
}

int saveFat(fat *fat) {
    // check if FAT is null
    if (fat == NULL) {
        printf("NULL FAT\n");
        return FAILURE;
    }

    // write directory file to disk
    if (writeDirectoryFile(fat) == FAILURE) {
        printf("Failed to write directory entries\n");
        return FAILURE;
    }

    return SUCCESS;
}

void freeFat(fat **fat) {
    // frees things carefully, allows us to call this function to free poorly formed FATs
    struct fatType *theFat = (*fat);

    // if the pointer is NULL, return
    if (theFat == NULL)
        return;

    // free fileName if allocated
    if (theFat->fileName != NULL)
        free(theFat->fileName);

    // free all directory entries
    while (theFat->firstDirectoryEntryNode != NULL) {
        directoryEntryNode *curr = theFat->firstDirectoryEntryNode;
        theFat->firstDirectoryEntryNode = curr->next;
        freeDirectoryEntryNode(curr);
    }

    // unmap FAT table
    if (munmap(theFat->blocks, theFat->numBlocks * theFat->blockSize) == -1) {
        perror("munmap");
        return;
    }

    free(theFat);

    // ensure passed in pointer is NULL
    *fat = NULL;
}