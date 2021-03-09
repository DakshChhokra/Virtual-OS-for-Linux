#ifndef FAT_H
#define FAT_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

/**
 * @file fat.h
 * @brief Alongside file.h, contains a FAT filesystem implementation along with functions for creating, saving and loading a FAT
 */

/**
 * A directory entry in PennFAT, encompassing 64 bytes and
 * can be directly written into the FAT file on disk
 */
typedef struct directoryEntryType {
    char name[32];
    uint32_t size;
    uint16_t firstBlock;
    uint8_t type;
    uint8_t perm;
    time_t mtime; // 8 bytes

    // 16 more bytes are reserved
    uint8_t reserved[16];
} directoryEntry;

/**
 * A linked list node containing a directory entry
 */
typedef struct directoryEntryNodeType {
    // the directory entry of this node
    directoryEntry *entry;

    // pointer to the next directory entry node
    struct directoryEntryNodeType *next;
} directoryEntryNode;

/**
 * @brief      Creates a new directory entry node
 *
 * @param      fileName    The file name
 * @param[in]  size        The size
 * @param[in]  firstBlock  The first block
 * @param[in]  type        The type
 * @param[in]  perm        The permission
 * @param[in]  time        The time
 *
 * @return     Pointer to a new directory entry node
 */
directoryEntryNode *newDirectoryEntryNode(
    char *fileName,
    uint32_t size,
    uint16_t firstBlock,
    uint8_t type,
    uint8_t perm,
    time_t time
);

/**
 * @brief      Frees a directory entry node
 *
 * @param      node  The directory entry node to free
 */
void freeDirectoryEntryNode(directoryEntryNode *node);

/**
 * FAT structure loaded and stored in memory
 */
typedef struct fatType {
    // Physical filename of the file system on the disk
    char *fileName;

    // Number of blocks this FAT takes up
    uint8_t numBlocks;

    // Number of bytes per block in this FAT
    uint32_t blockSize;

    // Number of entries in this FAT
    uint32_t numEntries;

    // Number of free blocks
    uint32_t freeBlocks;

    // Number of files/directories under the root directory
    uint32_t fileCount;

    // Linked list of directory entries
    directoryEntryNode *firstDirectoryEntryNode;
    // Last element in the linked list, useful for creating new files
    directoryEntryNode *lastDirectoryEntryNode;

    // Array of block links
    uint16_t *blocks;
} fat;

/**
 * @brief      Makes a PennFAT filesystem
 *             
 * @param[in]  fileName            The filename
 * @param[in]  numBlocks           The number of blocks in this FAT (1 - 32)
 * @param[in]  blockSizeIndicator  The size of each block (1 -> 512 bytes, 2 -> 1024 bytes, 3 -> 2048 bytes, 4 -> 4096 bytes)
 * @param[in]  creating            Whether or not we are creating a new FAT / overwriting existing FAT on disk
 *
 * @return     A pointer to the FAT stored in memory
 */
fat *getFat(char *fileName, uint8_t numBlocks, uint8_t blockSizeIndicator, bool creating);

/**
 * @brief      Loads a PennFAT filesystem from disk
 *
 * @param      fileName  The filename
 *
 * @return     A pointer to the loaded FAT stored in memory
 */
fat *loadFat(char *fileName);

/**
 * @brief      Saves a PennFAT filesystem to disk
 *
 * @param      fat   The FAT
 * 
 * @return     SUCCESS on successful save, FAILURE when syscalls fail or unable to write to disk
 */
int saveFat(fat *fat);

/**
 * @brief      Frees the FAT from memory and ensures the handler is NULL
 *
 * @param      fat   Pointer to the FAT pointer
 */
void freeFat(fat **fat);

#endif