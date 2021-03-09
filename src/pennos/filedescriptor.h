#ifndef FILE_DESC_H
#define FILE_DESC_H

#include "../fs/fat.h"

/**
 * @file filedescriptor.h
 * @brief Contains file descriptor abstraction and user-facing functions for file system interaction
 */

/**
 * A file descriptor node
 */
typedef struct fileDescriptorNodeType {
    // the id of this file descriptor
    int id;
    directoryEntry *entry;
    int mode;
    int pos;

    // pointer to the next file descriptor node
    struct fileDescriptorNodeType *next;
} fdNode;

/**
 * Stores file descriptors
 */
typedef struct FileDescriptorContainerType {
	fdNode *firstFdNode;
	fdNode *lastFdNode;
} fdContainer;

/**
 * Global container for file descriptors
 */
fdContainer *container;

/**
 * @brief      Instantiate a new file descriptor container
 *
 * @return     Pointer to the new container
 */
fdContainer *newContainer();

/**
 * @brief      Creates and adds a new file descriptor with specified filename and node and assigns an id
 *
 * @param      fileName   The file name for the descriptor
 * @param[in]  mode       The mode for the descriptor
 *
 * @return     Pointer to the new file descriptor node, or NULL if it failed
 */
fdNode *newFileDescriptorNode(char *fileName, int mode, directoryEntry *entry);

/// System calls using file descriptor abstractions

/**
 * @brief      Opens a file descriptor
 *
 * @param[in]  fname  The filename
 * @param[in]  mode   The mode
 *
 * @return     The ID of the file descriptor on success or FAILURE (-1) if it failed
 */
int f_open(char *fname, int mode);

/**
 * @brief      Read n bytes from fd into buf
 *
 * @param[in]  fd    The file descriptor
 * @param[in]  n     The number of bytes to read
 * @param      buf   The buffer to read into
 *
 * @return     The number of bytes read, 0 if EOF reached, or FAILURE (-1) on error
 */
int f_read(int fd, int n, uint8_t *buf);

/**
 * @brief      Write n bytes from buf into fd
 *
 * @param[in]  fd    The file descriptor
 * @param      buf   The buffer to read from
 * @param[in]  n     The number of bytes to write
 *
 * @return     The number of bytes written, or FAILURE (-1) on error
 */
int f_write(int fd, uint8_t *buf, int n);

/**
 * @brief      Close the file descriptor
 *
 * @param[in]  fd    The id of the file descriptor to close
 *
 * @return     SUCCESS (0) on success, FAILURE (-1) on failure
 */
int f_close(int fd);

/**
 * @brief      Rename the src file to dest
 *
 * @param      src   The source
 * @param      dest  The destination
 *
 * @return     SUCCESS (0) on success, FAILURE (-1) on failure
 */
int f_mv(char *src, char *dest);

/**
 * @brief      Remove a file with the specified filename
 *
 * @param      fileName  The file name
 *
 * @return     SUCCESS (0) on success, FAILURE (-1) on failure
 */
int f_unlink(char *fileName);

/**
 * @brief      Reposition the file position pointer for the specified file descriptor
 *
 * @param[in]  fd      The file descriptor
 * @param[in]  offset  The offset
 * @param[in]  whence  The whence
 *
 * @return     The new location of the file pointer on success, FAILURE (-1) on failure
 */
int f_lseek(int fd, int offset, int whence);

/**
 * @brief      Lists all the files in the system
 */
void f_ls();

/**
 * @brief      Changes the permission of a file
 *
 * @param      fileName    The file name
 * @param[in]  permission  The new permission
 *
 * @return     SUCCESS (0) on success, FAILURE (-1) on failure
 */
int f_chmod(char *fileName, int permission);

#endif