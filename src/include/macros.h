/**
 * @file  macros.h
 *
 * @brief Useful macros for code readability
 *
 */

#include "p_errno.h"


#define SUCCESS 0
#define FAILURE -1
#define RESET_ERRNO p_errno = 0;

#define PENNOS_PROMPT "penn-os> "
#define PENNOS_PROMPT_LENGTH 9

#define PENNFAT_PROMPT "pennfat> "
#define PENNFAT_PROMPT_LENGTH 9

#define DIRECTORY_FILENAME "/"

#define UNKNOWN_FILETYPE 0
#define REGULAR_FILETYPE 1
#define DIRECTORY_FILETYPE 2
#define SYMLINK_FILETYPE 4

#define NONE_PERMS 0
#define WRITE_PERMS 2
#define READ_PERMS 4
#define READWRITE_PERMS 6

#define S_SIGSTOP 0
#define S_SIGCONT 1
#define S_SIGTERM 2

#define W_WIFEXITED(status) (status == EXITED)
#define W_WIFSTOPPED(status) (status == STOPPED)
#define W_WIFSIGNALED(status) (status == SIGNALED)

#define F_WRITE 0
#define F_READ 1
#define F_APPEND 2

#define F_SEEK_SET 0
#define F_SEEK_CUR 1
#define F_SEEK_END 2

#define READY 0
#define BLOCKED 1
#define STOPPED 2
#define SIGNALED 3
#define EXITED 4
#define QUANTUM 100
