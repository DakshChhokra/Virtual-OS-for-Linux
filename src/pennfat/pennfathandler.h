#ifndef PENNFATHANDLER_H
#define PENNFATHANDLER_H

#include "../fs/fat.h"

/**
 * @file pennfathandler.h
 * @brief Contains functions that handle pennFat commands in the standalone shell
 */

/**
 * @brief      Handles pennfat commands
 *
 * @param      commands      The commands
 * @param[in]  commandCount  The command count
 * @param      fat           Pointer to the FAT pointer
 * 
 * @return     SUCCESS on success, FAILURE on failure to process command
 */
int  handlePennFatCommand(char ***commands, int commandCount, fat **fat);

/**
 * @brief      Handles mkfs command
 *
 * @param      fileName            The file name
 * @param[in]  numBlocks           The number blocks
 * @param[in]  blockSizeIndicator  The block size indicator
 * @param      fat                 Pointer to the FAT pointer
 *
 * @return     SUCCESS on success, FAILURE on invalid inputs or failure to allocate space
 */
int handleMakeFsCommand(char *fileName, uint8_t numBlocks, uint8_t blockSizeIndicator, fat **fat);

/**
 * @brief      Handles mount command
 *
 * @param      fileName  The file name on disk of the FAT
 * @param      fat       Pointer to the FAT pointer
 *
 * @return     SUCCESS on success, FAILURE on failure to allocate or any failed library calls
 */
int handleMountCommand(char *fileName, fat **fat);

/**
 * @brief      Handles unmount command
 * 
 * @param      fat  Pointer to the FAT pointer
 *
 * @return     SUCCESS on success, FAILURE if no FAT mounted
 */
int handleUnmountCommand(fat **fat);

/**
 * @brief      Handles touch command which creates a file if it doesn't exist, and updates the timestamp
 *
 * @param      files  The files to touch
 * @param      fat    The FAT pointer
 *
 * @return     SUCCESS on success, FAILURE on failure
 */
int handleTouchCommand(char **files, fat *fat);

/**
 * @brief      Handle move command which renames a file
 *
 * @param      oldFileName  The old file name
 * @param      newFileName  The new file name
 * @param      fat          The FAT pointer
 *
 * @return     SUCCESS on success, FAILURE on failure
 */
int handleMoveCommand(char *oldFileName, char *newFileName, fat *fat);

/**
 * @brief      Handle remove command which removes a file
 *
 * @param      files  The files to touch
 * @param      fat    The FAT pointer
 *
 * @return     SUCCESS on success, FAILURE on failure
 */
int handleRemoveCommand(char **files, fat *fat);

/**
 * @brief      Handle cat command
 *
 * @param      commands  The commands to parse and define behavior on
 * @param      fat       The FAT pointer
 *
 * @return     SUCCESS on success, FAILURE on failure
 */
int handleCatCommand(char **commands, fat *fat);

/**
 * @brief      Handle copy command
 *
 * @param      commands  The commands to parse and define behavior on
 * @param      fat       The FAT pointer
 *
 * @return     SUCCESS on success, FAILURE on failure
 */
int handleCopyCommand(char **commands, fat *fat);

/**
 * @brief      Prints all of the files in the directory
 * 
 * @param      fat  The FAT pointer
 *
 * @return     SUCCESS on success, FAILURE if no FAT is mounted
 */
int handleLsCommand(fat *fat);

/**
 * @brief      Changes the permissions of a file
 *
 * @param      commands  The commands
 * @param      fat       The fat
 *
 * @return     SUCCESS on success, FAILURE if no FAT is mounted
 */
int handleChmodCommand(char **commands, fat *fat);

#endif