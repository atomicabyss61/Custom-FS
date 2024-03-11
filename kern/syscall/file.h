#ifndef FILE_H
#define FILE_H

/* System call prototypes */

/* opens a file with specific flags and modes */
int sys_open(char *filename, int flags, mode_t mode, uint32_t *v0);

/* closes a file descriptor and relative open file if need be */
int sys_close(int fd);

/* writes a buffer to a file */
ssize_t sys_write(int fd, userptr_t a1, size_t nbytes, int *err);

/* reads a buflen amount of characters from a file */
ssize_t sys_read(int fd, userptr_t buf, size_t buflen, int *err);

/* dups newfd onto oldfd (arguments in reverse order of actual dup2) */
int sys_dup2(int newfd, int oldfd, uint32_t *v0);

/* changes the file pointer to the correct offset specified based on placement flags */
int sys_lseek(int fd, uint32_t hi, uint32_t lo, userptr_t poi, uint32_t *v0, uint32_t *v1);

#endif