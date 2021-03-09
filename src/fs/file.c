#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "fat.h"
#include "file.h"
#include "../include/macros.h"

int bytesToBlocks(int numBytes, fat *fat) {
    return ceil((double) numBytes / fat->blockSize);
}

void freeFile(file *file) {
    free(file->bytes);
    free(file);
}

void getEntryNodeAndPrev(directoryEntryNode **prev, directoryEntryNode **found, char *fileName, fat *fat) {
    directoryEntryNode *prevNode;
    directoryEntryNode *entryNode = fat->firstDirectoryEntryNode;
    
    if (fileName == NULL)
        return;

    while (entryNode != NULL) {
        if (strcmp(entryNode->entry->name, fileName) == 0) {
            break;
        } else {
            prevNode = entryNode;
            entryNode = entryNode->next;
        }
    }

    if (prev != NULL)
        *prev = prevNode;
    if (found != NULL)
        *found = entryNode;
}

file *getDirectoryFile(fat *fat) {
    // get number of files in the root directory
    // open the file
    int fd;
    if ((fd = open(fat->fileName, O_RDWR, 0644)) == -1) {
        perror("open");
        return NULL;
    }

    // seek to the first block
    uint32_t fatSize = fat->numBlocks * fat->blockSize;
    uint16_t currIndex = 1;

    if (lseek(fd, fatSize, SEEK_SET) == -1) {
        perror("lseek");
        return NULL;
    }

    // keep track of how many files we counted
    unsigned int filesCounted = 0;

    // temporary buffer to store bytes read
    uint8_t buffer[sizeof(directoryEntry)];
    for (int i = 0; i < sizeof(directoryEntry); i++)
        buffer[i] = 0x00;

    // read 64 bytes at a time, finding a new block every time we read (blockSize) bytes until
    // we read a file where the first byte is NULL
    
    bool endedAtEdgeOfBlock = false;
    while (1) {
        if (filesCounted != 0 && (filesCounted * sizeof(directoryEntry)) % fat->blockSize == 0) {
            if (fat->blocks[currIndex] == 0xFFFF) {
                // if we have reached the end of a block and this is the last block, then we are done
                endedAtEdgeOfBlock = true;
                break;
            }
            // get next block to start reading from
            currIndex = fat->blocks[currIndex];
            if (lseek(fd, fatSize + ((currIndex - 1) * fat->blockSize), SEEK_SET) == -1) {
                perror("lseek");
                return NULL;
            }
        }

        // read 64 bytes into the buffer
        if (read(fd, buffer, sizeof(directoryEntry)) == -1) {
            perror("read");
            return NULL;
        }

        // check if the first byte was null, if so, break
        // otherwise continue reading and incr filesCounted
        if (buffer[0] == 0x00)
            break;
        else
            filesCounted++;
    }

    // close fd
    if (close(fd) == -1) {
        perror("close");
        return NULL;
    }

    // return NULL if no files found, otherwise, copy the first filesCounted * 64 bytes to heap and return
    if (filesCounted == 0) {
        return NULL;
    } else {
        uint8_t *bytes = getBytes(1, filesCounted * sizeof(directoryEntry) + endedAtEdgeOfBlock, fat);

        // check if getBytes failed
        if (bytes == NULL) {
            perror("malloc");
            return NULL;
        }

        // formulate output file
        file *out = malloc(sizeof(file));

        // set file details
        out->bytes = bytes;
        out->len = filesCounted * sizeof(directoryEntry);
        out->type = DIRECTORY_FILETYPE;
        out->perm = NONE_PERMS;

        return out;
    }
}

uint8_t *getBytes(uint16_t startIndex, uint32_t length, fat *fat) {
    uint8_t *result = malloc(length * sizeof(uint8_t) + 1);
    // check if malloc failed
    if (result == NULL) {
        perror("malloc");
        return NULL;
    }

    // null terminate this byte array
    result[length] = '\0';

    // read bytes from FAT storage for each block of this file
    // open the file to read from and check for errors
    int fd;
    if ((fd = open(fat->fileName, O_RDONLY, 0644)) == -1) {
        perror("open");
        free(result);
        return NULL;
    }

    // seek to the first empty block
    uint32_t fatSize = fat->numBlocks * fat->blockSize;
    uint16_t currIndex = startIndex;

    if (lseek(fd, fatSize + ((currIndex - 1) * fat->blockSize), SEEK_SET) == -1) {
        perror("lseek");
        free(result);
        return NULL;
    }

    // read all (length) bytes, finding a new block every time we read (blockSize) bytes
    for (int i = 0; i < length; i = i + fat->blockSize) {
        if (i != 0 && i % fat->blockSize == 0) {
            // get next block to start reading from
            currIndex = fat->blocks[currIndex];
            if (lseek(fd, fatSize + ((currIndex - 1) * fat->blockSize), SEEK_SET) == -1) {
                perror("lseek");
                free(result);
                return NULL;
            }
        }

        // read the smaller of blockSize or length - i;
        int bytesToRead = fat->blockSize;
        if (bytesToRead > length - i)
            bytesToRead = length - i;

        if (read(fd, &result[i], bytesToRead) == -1) {
            perror("read");
            free(result);
            return NULL;
        }
    }

    // close the file
    if (close(fd) == -1) {
        perror("close");
        free(result);
        return NULL;
    }

    return result;
}

file *readFileFromFAT(char *fileName, fat *fat) {
    // find the directory entry that matches this filename, if it exists
    directoryEntryNode *entryNode;
    getEntryNodeAndPrev(NULL, &entryNode, fileName, fat);

    // check if we found a node corresponding to the supplied fileName
    if (entryNode == NULL) {
        printf("%s not found\n", fileName);
        return NULL;
    }

    if (entryNode->entry->perm != READWRITE_PERMS && entryNode->entry->perm != READ_PERMS) {
        printf("%s lacks read permission\n", fileName);
        return NULL;
    }

    // allocate space for this file
    file *out = malloc(sizeof(file));

    // check if malloc failed
    if (out == NULL)
        return NULL;

    out->bytes = getBytes(entryNode->entry->firstBlock, entryNode->entry->size, fat);

    // check if getBytes failed
    if (out->bytes == NULL) {
        free(out);
        return NULL;
    }
    out->len = entryNode->entry->size;
    out->type = entryNode->entry->type;
    out->perm = entryNode->entry->perm;
    // temporarily return NULL
    return out;
}

/**
 * @brief      Helper to delete the block links associated with an entryNode
 *
 * @param      prev       The node immediately previous to the entry node, NULL if it doesn't exist
 * @param      entryNode  The entry node whose block links to delete
 * @param      fat        The FAT filesystem
 * @param      dirFile    Only true when writing directory file
 */
void deleteFileHelper(directoryEntryNode *prev, directoryEntryNode *entryNode, fat *fat, bool dirFile) {
    // clear blocks in FAT
    uint16_t currBlock;
    if (dirFile) {
        // handle case for overwriting directory file
        currBlock = 1;
    } else {
        currBlock = entryNode->entry->firstBlock;
    }

    if (dirFile || entryNode->entry->size != 0) {
        // delete all blocks for this file
        do {
            // get next block
            uint16_t nextBlock = fat->blocks[currBlock];
            // clear current block
            fat->blocks[currBlock] = 0;
            // set next block as current block
            currBlock = nextBlock;
        } while (currBlock != 0xFFFF && currBlock != 0x0000);
    }
}

int deleteFileFromFAT(char *fileName, fat *fat, bool syscall) {
    // find the directory entry that matches this filename, if it exists
    directoryEntryNode *prev;
    directoryEntryNode *entryNode;
    getEntryNodeAndPrev(&prev, &entryNode, fileName, fat);

    // check if we found a node corresponding to the supplied fileName
    if (entryNode == NULL) {
        printf("%s not found\n", fileName);
        return FAILURE;
    }

    // check if the file has write permissions
    if (!syscall && entryNode != NULL && entryNode->entry->perm != WRITE_PERMS && entryNode->entry->perm != READWRITE_PERMS) {
        printf("%s lacks write permission\n", fileName);
        return FAILURE;
    }

    // delete block links
    deleteFileHelper(prev, entryNode, fat, false);

    // delete this entry node from the linked list
    // set first node pointer to the next entry if this entry is the first entry
    // otherwise set previous entry's next to this entry's next
    if (entryNode == fat->firstDirectoryEntryNode)
        fat->firstDirectoryEntryNode = entryNode->next;
    else
        prev->next = entryNode->next;

    // set last node pointer to the prev entry if this entry is the last entry
    if (entryNode == fat->lastDirectoryEntryNode)
        fat->lastDirectoryEntryNode = prev;

    // decrement filecount and increase freeblocks by the number of blocks taken up by the file
    fat->fileCount--;
    fat->freeBlocks += bytesToBlocks(entryNode->entry->size, fat);

    // free this node
    freeDirectoryEntryNode(entryNode);

    return SUCCESS;
}

int renameFile(char *oldFileName, char *newFileName, fat *fat) {
    // get directory entry for filename
    directoryEntryNode *entryNode;
    getEntryNodeAndPrev(NULL, &entryNode, oldFileName, fat);

    // fail if src file doesn't exist
    if (entryNode == NULL) {
        printf("%s does not exist\n", oldFileName);
        return FAILURE;
    }

    // fail if the src file does not have write permission
    if (entryNode->entry->perm == NONE_PERMS || entryNode->entry->perm == READ_PERMS) {
        printf("%s lacks write permission\n", oldFileName);
        return FAILURE;
    }

    // check if file with newFileName already exists
    directoryEntryNode *newFileNode;
    getEntryNodeAndPrev(NULL, &newFileNode, newFileName, fat);

    // if so, try to delete it
    if (newFileNode != NULL) {
        if (deleteFileFromFAT(newFileNode->entry->name, fat, false) == FAILURE) {
            printf("Failed to overwrite %s\n", newFileNode->entry->name);
        }
    }

    // rename file
    strcpy(entryNode->entry->name, newFileName);

    // update timestamp
    entryNode->entry->mtime = time(NULL);

    // update timestamp of directory file
    fat->firstDirectoryEntryNode->entry->mtime = entryNode->entry->mtime;

    return SUCCESS;
}

int writeFileToFAT(char *fileName, uint8_t *bytes, uint32_t fileOffset, uint32_t length, uint8_t type, uint8_t perm, fat *fat, bool appending, bool syscall, bool writeDir) {
    // find the directory entry that matches this filename, if it exists
    directoryEntryNode *prev;
    directoryEntryNode *entryNode;
    getEntryNodeAndPrev(&prev, &entryNode, fileName, fat);

    // check if the file has write permissions
    if (!syscall && entryNode != NULL && entryNode->entry->perm != WRITE_PERMS && entryNode->entry->perm != READWRITE_PERMS) {
        printf("%s lacks write permission\n", fileName);
        return FAILURE;
    }

    // calculate the number of free blocks taken or created by this write
    int32_t changeInFreeBlocks = 0;

    // required blocks depends on whether we should create a new directory entry, if we're appending, or just overwriting
    if (syscall && writeDir) {
        // will not change free blocks
    } else if (entryNode == NULL) {
        // if the directory file needs to expand, then we need an extra block
        if (fat->fileCount != 0 && (sizeof(directoryEntry) * fat->fileCount) % fat->blockSize == 0)
            changeInFreeBlocks -= 1;
        
        // need enough free blocks for the content of the file
        changeInFreeBlocks -= bytesToBlocks(length, fat);
    } else if (appending) {
        // need enough free blocks for the content of the file minus the free space in the last block of the file
        if (entryNode->entry->size % fat->blockSize == 0)
            changeInFreeBlocks -= bytesToBlocks(length, fat);
        else
            changeInFreeBlocks -= bytesToBlocks(length - (fat->blockSize - (entryNode->entry->size % fat->blockSize)), fat);
    } else if (fileOffset > 0) {
        // change in free blocks is (required blocks for new file) - (freed blocks from old file)
        changeInFreeBlocks -= bytesToBlocks(fileOffset + length, fat) - bytesToBlocks(entryNode->entry->size, fat);
    } else {
        // change in free blocks is (required blocks for new file) - (freed blocks from old file)
        changeInFreeBlocks -= bytesToBlocks(length, fat) - bytesToBlocks(entryNode->entry->size, fat);
    }

    // fail if not enough space
    if ((int32_t) fat->freeBlocks + changeInFreeBlocks < 0) {
        printf("Not enough free blocks, %d blocks required, %d blocks free\n", -changeInFreeBlocks, fat->freeBlocks);
        return FAILURE;
    }

    // delete original file's block links, if it exists and we are not appending
    if (writeDir || (entryNode != NULL && !appending)) {
        deleteFileHelper(prev, entryNode, fat, syscall && writeDir);
    }

    // get the index of the first free block or the last block of the file if appending to nonempty file and file does not
    // perfectly fill up the last block, i.e. length % blocksize != 0
    uint16_t currIndex = 1;

    // offset from the first block's first byte to write at, zero if empty file or new file, size % blocksize if nonempty and appending
    uint32_t offset = 0;

    if (syscall && writeDir) {
        // curr index will be 1 and offset 0 if writing directory file
        currIndex = 1;
        offset = 0;
    } else if (appending && entryNode != NULL && entryNode->entry->size != 0) {
        // get the last block of the file to append to
        currIndex = entryNode->entry->firstBlock;
        while (fat->blocks[currIndex] != 0xFFFF) {
            currIndex = fat->blocks[currIndex];
        }

        if (entryNode->entry->size % fat->blockSize == 0) {
            // find free free block and set current block pointer to that block
            uint16_t nextIndex = 0;
            for (unsigned int i = 1; i < fat->numEntries; i++) {
                if (fat->blocks[i] == 0) {
                    nextIndex = i;
                    break;
                }
            }
            fat->blocks[currIndex] = nextIndex;
            currIndex = nextIndex;
        }

        // get current offset of file's end
        offset = entryNode->entry->size % fat->blockSize;
    } else if (fileOffset > 0 && entryNode != NULL) {
        if (fileOffset > entryNode->entry->size) {
            printf("Cannot write to at offset greater than file length\n");
            return FAILURE;
        }

        // get the block where the offset lies in
        currIndex = entryNode->entry->firstBlock;

        int blockAt = fileOffset / fat->blockSize; 

        for (int i = 0; i < blockAt; i++) {
            currIndex = fat->blocks[currIndex];
        }

        if (entryNode->entry->size % fat->blockSize == 0) {
            // find free free block and set current block pointer to that block
            uint16_t nextIndex = 0;
            for (unsigned int i = 1; i < fat->numEntries; i++) {
                if (fat->blocks[i] == 0) {
                    nextIndex = i;
                    break;
                }
            }
            fat->blocks[currIndex] = nextIndex;
            currIndex = nextIndex;
        }

        // get current offset of file's end
        offset = fileOffset % fat->blockSize;
    } else {
        // get first free block
        for (unsigned int i = 1; i < fat->numEntries; i++) {
            if (fat->blocks[i] == 0) {
                currIndex = i;
                break;
            }
        }
    }

    // store the first index of this file
    uint16_t firstIndex = currIndex;

    // write bytes to FAT storage for each open block
    // open the file to write to and check for errors
    int fd;
    if ((fd = open(fat->fileName, O_WRONLY | O_CREAT, 0644)) == -1) {
        perror("open");
        return FAILURE;
    }
    // seek to the first empty block
    uint32_t fatSize = fat->numBlocks * fat->blockSize;
    if (lseek(fd, fatSize + ((currIndex - 1) * fat->blockSize) + offset, SEEK_SET) == -1) {
        perror("lseek");
        return FAILURE;
    }

    // write all (length) bytes in as few write calls as possible, finding a new block every time when necessary
    int byteIdx = 0;
    while (byteIdx < length) {
        if (byteIdx != 0 && (byteIdx + offset) % fat->blockSize == 0) {
            if (fat->blocks[currIndex] == 0x0000 || fat->blocks[currIndex] == 0xFFFF) {
                // find new block and start writing in new block
                uint16_t nextIndex = currIndex + 1;
                // check if nextIndex is out of bounds, if so, wrap around to the second block of data section
                if (nextIndex >= fat->numEntries)
                    nextIndex = 2;

                for (unsigned int j = nextIndex; j < fat->numEntries; j++) {
                    if (fat->blocks[j] == 0) {
                        nextIndex = j;
                        break;
                    }
                }
                fat->blocks[currIndex] = nextIndex;
                currIndex = nextIndex;
                if (lseek(fd, fatSize + ((currIndex - 1) * fat->blockSize), SEEK_SET) == -1) {
                    perror("lseek");
                    return FAILURE;
                }   
            } else {
                currIndex = fat->blocks[currIndex];
            }
        }

        // write either enough bytes to get to the end of this block or the number of bytes to the end of the file,
        // whichever is smaller
        int bytesToWrite = fat->blockSize - (byteIdx + offset) % fat->blockSize;
        if (bytesToWrite > length - byteIdx)
            bytesToWrite = length - byteIdx;


        // in case successful write doesn't write all the bytes
        int totalBytesWritten = 0;
        while (totalBytesWritten < bytesToWrite) {
            int bytesWritten = write(fd, &bytes[byteIdx], bytesToWrite);

            if (bytesWritten == -1) {
                perror("write");
                return FAILURE;
            }

            totalBytesWritten += bytesWritten;
        }

        byteIdx = byteIdx + bytesToWrite;
    }

    // set final block's next block link to be 0xFFFF i.e. end of file, if the length is nonzero
    if (length != 0)
        fat->blocks[currIndex] = 0xFFFF;
    else
        firstIndex = 0x0000;

    // close and save the file
    if (close(fd) == -1) {
        perror("close");
        return FAILURE;
    }

    // create a new directory entry and node if it was NULL
    if (syscall && writeDir) {
        // do nothing -- we just needed to update the file on disk
    } else if (entryNode == NULL) {
        directoryEntryNode *newNode = newDirectoryEntryNode(fileName, length, firstIndex, type, perm, time(NULL));

        // check if list has been initialized
        if (fat->fileCount == 0) {
            // initialize linked list
            fat->firstDirectoryEntryNode = newNode;
            fat->lastDirectoryEntryNode = newNode;
        } else {
            // add new entry to end of list
            fat->lastDirectoryEntryNode->next = newNode;
            fat->lastDirectoryEntryNode = newNode;
        }

        // update file count free space
        fat->fileCount++;
    } else {
        // update existing entry size
        if (fileOffset > 0 && fileOffset + length > entryNode->entry->size) {
            entryNode->entry->size = fileOffset + length;
        } else if (appending) {
            entryNode->entry->size += length;
        } else {
            entryNode->entry->size = length;
        }
        if (!appending)
            entryNode->entry->firstBlock = firstIndex;
        entryNode->entry->mtime = time(NULL);
    }

    // update number of free blocks
    fat->freeBlocks += changeInFreeBlocks;

    return SUCCESS;
}

int appendToFileInFAT(char *fileName, uint8_t *bytes, uint32_t length, fat *fat, bool syscall) {
    return writeFileToFAT(fileName, bytes, 0, length, REGULAR_FILETYPE, READWRITE_PERMS, fat, true, syscall, false);
}

int writeDirectoryFile(fat *fat) {
    // gather bytes to write (including a null byte after the end of the last entry if the number of files does not perfectly fill up a block)
    uint32_t fileSize = fat->fileCount * sizeof(directoryEntry);
    uint32_t length = fileSize;
    if (fileSize % fat->blockSize != 0)
        length += 1;

    uint8_t bytes[length];

    if (fileSize % fat->blockSize != 0)
        bytes[length - 1] = 0x00;

    int bufIdx = 0;
    directoryEntryNode *entryNode = fat->firstDirectoryEntryNode;
    while (entryNode != NULL) {
        // copy entry bytes into buffer
        memcpy(&bytes[bufIdx], entryNode->entry, sizeof(directoryEntry));
        entryNode = entryNode->next;
        bufIdx += sizeof(directoryEntry);
    }


    if (writeFileToFAT(NULL, bytes, 0, length, DIRECTORY_FILETYPE, NONE_PERMS, fat, false, true, true) == -1) {
        printf("Failed to save directory file\n");
        return FAILURE;
    }

    return SUCCESS;
}

int chmodFile(fat *fat, char *fileName, int newPerms) {
    directoryEntryNode *foundNode;
    getEntryNodeAndPrev(NULL, &foundNode, fileName, fat);

    if (foundNode == NULL) {
        printf("%s not found\n", fileName);
        return FAILURE;
    }

    if (newPerms != NONE_PERMS && newPerms != WRITE_PERMS && newPerms != READ_PERMS && newPerms != READWRITE_PERMS) {
        printf("%02x is an invalid permission\n", newPerms);
        return FAILURE;
    }

    foundNode->entry->perm = newPerms;

    return SUCCESS;
}
