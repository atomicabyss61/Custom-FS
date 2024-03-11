#ifndef FDTABLE_H
#define FDTABLE_H

#include <syscall.h>
#define  TABLE_LIM 128

/* FD Table specific to process */
int *fd;

/**
 * OF Table entry struct
 * 
 * contains:
 * -    flag, gives file permissions and commands.
 * -    vptr, pointer to the vnode struct of a file.
 * -    fp, gives the place which the file pointer is at.
 * -    fd_count, gives the amount of file descriptors pointing to an open file.
 * 
 */
struct of_item {
    int flag;
    struct vnode *vptr;
    off_t fp;
    int fd_count;
};

/* Global OF Table*/
struct of_item *of_table[TABLE_LIM];

/* OF Table operations */

/* Intialises each index in Open File table to NULL */
void _of_init(void);

/* Frees everything in the Open File table */
void _of_destroy(void);

/* Creates an Open File table entry at an index and intialises the values */
int _of_create(int flags, struct vnode *vptr, int *result);

/* Returns the file pointer at index in Open File table 
   if file descriptor has correct permissions */
off_t _of_flag(int of_ind, int flag, int *result);

/* Returns the vptr from an index in the Open File table */
struct vnode *_of_vnode(int of_ind);

/* Takes the file pointer at index in Open File table and increments by nbytes */
int _of_progress(int of_ind, off_t nbytes);

/* Adds the file pointer at index in Open File table to an offset inputted */
int _of_set_fp(int of_ind, off_t offset);


/* FD Table operations */

/* Intialises the components for File Descriptor table */
void _fd_init(void);

/* Frees everything in the File Descriptor table  */
void _fd_destroy(void);

/* Creates and sets a File Descriptor index to point at an OF Table entry*/
int _fd_create(int of_ind, int *result);

/* Closes the file inputed in the File Descriptor table */
int _fd_close(int fd_ind);

/* Checks for invalid file descriptor or return file descriptor index
   in File Descriptor table  */
int _fd_point(int fd_ind, int *result);

/* Returns the file descriptor for the new file destination inputted */
int _fd_allocate(int dest_of, int *result);



#endif 