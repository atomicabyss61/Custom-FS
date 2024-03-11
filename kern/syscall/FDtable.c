#include <syscall.h>
#include "FDtable.h"

void
_of_init(void) {
    
    for (int i = 0; i < TABLE_LIM; i++) {
        of_table[i] = NULL;
    }

}

void
_of_destroy(void) 
{
    for (int i = 0; i < TABLE_LIM; i++) {
        kfree(of_table[i]);
    }
    kfree(of_table);
}

int
_of_create(int flags, struct vnode *vptr, int *result)
{   

    int ind = 0;
    
    /* Finding lowest of table entry */
    for (; ind < TABLE_LIM; ind++) {
        if (of_table[ind] == NULL) {
            break;
        } 
    }

    if (ind == TABLE_LIM) {
        /* No space left in of table */
        *result = EMFILE;
        return 0;
    }

    of_table[ind] = kmalloc(sizeof(struct of_item *));

    if (of_table[ind] == NULL) {
        /* failed to allocate memory error */
        *result = ENOMEM;
        return 0;
    }

    /* Setting flags and ptrs */
    of_table[ind]->flag = flags & 0b11;
    of_table[ind]->vptr = vptr;
    of_table[ind]->fp = 0;
    of_table[ind]->fd_count = 1; 

    return ind;

}

off_t
_of_flag(int of_ind, int flag, int *result)
{

    if (of_ind < 0 || of_ind >=  TABLE_LIM) {
        /* invalid of value */
        *result = EBADF;
        return 0;
    }
    
    if (of_table[of_ind] == NULL) {
        /* non-existant table entry in that place */
        *result = EBADF;
        return 0;
    }


    if (flag == O_RDWR) {
        return of_table[of_ind]->fp;
    } else if (of_table[of_ind]->flag == O_RDWR) {
        return of_table[of_ind]->fp;
    } else if (of_table[of_ind]->flag != flag) {
        /*  file doesn't have correct permissions for operation */ 
        *result = EBADF;
        return 0;
    }
    
    return of_table[of_ind]->fp;
}

struct vnode *_of_vnode(int of_ind)
{

    if (of_table[of_ind] == NULL) {
        return NULL;
    }

    return of_table[of_ind]->vptr;

}


int
_of_progress(int of_ind, off_t nbytes)
{

    if (of_table[of_ind] == NULL) {
        return EBADF;
    }

    if (nbytes < 0) {
        return EBADF;
    }

    /* incrementing file pointer */
    of_table[of_ind]->fp = nbytes + of_table[of_ind]->fp;

    return 0;

}

int
_of_set_fp(int of_ind, off_t offset)
{

    if (of_table[of_ind] == NULL) {
        return EBADF;
    }

    if (offset < 0) {
        return EBADF;
    }

    of_table[of_ind]->fp = offset;

    return 0;

}

/* FD Table operations */

void 
_fd_init(void)
{
    fd = kmalloc(sizeof(int) * TABLE_LIM);

    if (fd == NULL) {
        panic("not enough memory for fd table");
    }

    for (int i = 0; i < TABLE_LIM; i++) {
        /* -1 is a null value for OF table */
        fd[i] = -1;
    }

    /* reserve first 3 fd */
    fd[0] = 0;
    fd[1] = 1;
    fd[2] = 2;

    /* Setting stdout and stderr */
    char conname[5];
    int result = 0;
    struct vnode *vptr;
    mode_t mode = 777;

    strcpy(conname,"con:");
    result = vfs_open(conname, O_RDONLY, mode, &vptr);

    if (result == -1) {
        return;
    }

    of_table[0] = kmalloc(sizeof(struct of_item *));
    of_table[0]->flag = O_RDONLY;
    of_table[0]->vptr = vptr;
    of_table[0]->fp = 0;
    of_table[0]->fd_count = 1;

    strcpy(conname,"con:");
    result = vfs_open(conname, O_WRONLY, mode, &vptr);

    if (result == -1) {
        return;
    }

    of_table[1] = kmalloc(sizeof(struct of_item *));
    of_table[1]->flag = O_WRONLY;
    of_table[1]->vptr = vptr;
    of_table[1]->fp = 0;
    of_table[1]->fd_count = 1;

    strcpy(conname,"con:");
    result = vfs_open(conname, O_WRONLY, mode, &vptr);

    if (result == -1) {
        return;
    }

    of_table[2] = kmalloc(sizeof(struct of_item *));
    of_table[2]->flag = O_WRONLY;
    of_table[2]->vptr = vptr;
    of_table[2]->fp = 0;
    of_table[2]->fd_count = 1; 

}

void 
_fd_destroy(void)
{
    kfree(fd);
}

int
_fd_create(int of_ind, int *result)
{

    int fd_ind = 0;

    for (; fd_ind < TABLE_LIM; fd_ind++) {
        if (fd[fd_ind] == -1) break;
    }

    if (fd_ind == TABLE_LIM) {
        /* errno number for not enough space in fd table */
        *result = ENOMEM;
        return 0;
    }

    fd[fd_ind] = of_ind;

    return fd_ind;

}

int
_fd_close(int fd_ind) 
{   

    int of_ind;
    int result = 0;

    /* finds the OF Table entry fd_ind points to */
    of_ind = _fd_point(fd_ind, &result); 

    if (result) {
        return result;
    }

    /* closes the file of fd_ind points to an open file */
    if (of_table[of_ind]->fd_count == 1) {
        struct vnode *vptr;
        vptr = _of_vnode(of_ind);

        vfs_close(vptr);

        kfree(of_table[of_ind]);
        of_table[of_ind] = NULL;
        fd[fd_ind] = -1;

    } else {

        of_table[of_ind]->fd_count--;
        fd[fd_ind] = -1;

    }

    return 0; 
}

int
_fd_point(int fd_ind, int *result)
{

    if (fd_ind < 0 || fd_ind >=  TABLE_LIM) {
        /* invalid fd value */
        *result = EBADF;
        return -1;
    }

    if (fd[fd_ind] == -1) {
        /* non-existant file in that place */
        *result = EBADF;
        return 0;
    }

    return fd[fd_ind];

}

int
_fd_allocate(int dest_of, int *result)
{

    int fd_ind;

    fd_ind = _fd_create(dest_of, result);

    if (*result) {
        /* process file table was full */
        *result = EMFILE;
        return *result;
    }

    return fd_ind;

}


