#ifndef FILE_H
#define FILE_H

#include <stdbool.h>
#include "fat.h"

/**
 * @file file.h
 * @brief Contains a file struct and functions that allow users to read, write, and modify files in a FAT filesystem and
 * additional functions for interacting with a PennFAT filesystem
 */

/**
 * File struct containing an array of bytes and the length of that array
 */
typedef struct fileType {
    uint8_t *bytes;
    unsigned int len;
    uint8_t type;
    uint8_t perm;
} file;

/**
 * @brief      Frees a file struct pointer and its bytes
 *
 * @param      file  The file
 */
void freeFile(file *file);

/**
 * @brief      Find a directory entry node for a certain filename
 *
 * @param      prev      Pointer to the pointer that will store the node before the found node, pass NULL if unused
 * @param      found     Pointer to the pointer that will store the found node, pass NULL if unused
 * @param      fileName  The file name to search for
 * @param      fat       The FAT
 */
void getEntryNodeAndPrev(directoryEntryNode **prev, directoryEntryNode **found, char *fileName, fat *fat);

/**
 * @brief      Helper function to get the root directory file in a FAT
 *
 * @param      fat   The fat
 *
 * @return     The root directory file
 */
file *getDirectoryFile(fat *fat);

/**
 * @brief      Helper function to get the bytes of a file starting at some index
 *
 * @param[in]  startIndex  The start index of the file
 * @param      length      The length of the file
 * @param      fat         The FAT
 *
 * @return     A null-terminated byte array representing the file
 */
uint8_t *getBytes(uint16_t startIndex, uint32_t length, fat *fat);

/**
 * @brief      Reads a file as byte pointers
 *
 * @param      fileName   The file name to write to
 * @param      fat        The FAT
 * @param      readDir    Whether or not we are reading the directory file -- only called by the system
 * 
 * @return     Returns a pointer to the null-terminated array of bytes
 */
file *readFileFromFAT(char *fileName, fat *fat);

/**
 * @brief      Delete a file from a FAT filesystem
 *
 * @param      fileName  The file name
 * @param      fat       The FAT
 * @param      syscall   Whether or not this call is allowed to modify files no matter the permissions
 *
 * @return     -1 (FAILURE) on failure, 0 (SUCCESS) on success
 */
int deleteFileFromFAT(char *fileName, fat *fat, bool syscall);

/**
 * @brief      Rename a file and update the last modified time
 *
 * @param      oldFileName  The old file name
 * @param      newFileName  The new file name
 * @param      fat  	    The FAT
 *
 * @return     SUCCESS on successful rename, FAILURE when no file is found or newFileName already exists
 */
int renameFile(char *oldFileName, char *newFileName, fat *fat);

/**
 * @brief      Writes (or overwrites) a file in a FAT filesystem
 *
 * @param      fileName   The file name of the file to write to
 * @param      bytes      The bytes to write
 * @param      offset     The byte offset to start writing at if not in append mode
 * @param[in]  length     The number of bytes to write
 * @param      type       The type of the file
 * @param      perm       The permissions of this file
 * @param      fat        The FAT filesystem
 * @param      appending  Whether or not to append to the file
 * @param      syscall    Whether or not this call is allowed to modify files no matter the permissions
 * @param      writeDir   Only used with system calls -- allows writing to the directory file
 *
 * @return     -1 (FAILURE) on failure, 0 (SUCCESS) on success
 */
int writeFileToFAT(char *fileName, uint8_t *bytes, uint32_t offset, uint32_t length, uint8_t type, uint8_t perm, fat *fat, bool appending, bool syscall, bool writeDir);

/**
 * @brief      Appends to file in a FAT filesystem.
 *
 * @param      fileName  The file name of the file to append to
 * @param      bytes     The bytes to append
 * @param[in]  length    The number of bytes to append
 * @param      fat       The FAT filesystem
 * @param      syscall   Whether or not this call is allowed to modify files no matter the permissions
 *
 * @return     -1 (FAILURE) on failure, 0 (SUCCESS) on success
 */
int appendToFileInFAT(char *fileName, uint8_t *bytes, uint32_t length, fat *fat, bool syscall);

/**
 * @brief      Writes the directory file
 *
 * @param      fat   The FAT filesystem
 *
 * @return     -1 (FAILURE) on failure, 0 (SUCCESS) on success
 */
int writeDirectoryFile(fat *fat);

/**
 * @brief      Changes the access permissions of the file
 *
 * @param      fat       The fat
 * @param      fileName  The file name
 * @param[in]  newPerms  The new permissions
 *
 * @return     -1 (FAILURE) on failure, 0 (SUCCESS) on success
 */
int chmodFile(fat *fat, char *fileName, int newPerms);

#endif