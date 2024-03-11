#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>

/* My files */
#include "FDtable.c"
#include "file.h"
#include <endian.h>

int
sys_open(char *filename, int flags, mode_t mode, uint32_t *v0)
{
  struct vnode *vptr;
  int result = 0;

  result = vfs_open(filename, flags, mode, &vptr);

  if (result) {
    /* vfs_open return error */
    return result;
  }

  int of_ind;

  of_ind = _of_create(flags, vptr, &result);

  if (result) {
    /* set to correct errno value */
    return result;
  }

  int fd_ind;

  fd_ind = _fd_create(of_ind, &result);

  if (result) {
    /* set to correct errno value */
    return result;
  }

  /* setting return value */
  *v0 = fd_ind;

  return 0;

}

int
sys_close(int fd)
{

  int result = 0;

  result = _fd_close(fd);

  if (result) {
    return result; 
  }
  
  return 0; 
}

ssize_t
sys_write(int fd, userptr_t a1, size_t nbytes, int *err)
{

  int of_ind;

  of_ind = _fd_point(fd, err);

  if (*err) {
    /* invalid fd */
    return -1;
  }

  off_t fp;
  
  fp = _of_flag(of_ind, O_WRONLY, err);
  
  if (*err) {
    /* insufficient user permissions */
    return -1;
  }

  if (nbytes == 0) {
    return 0;
  }

  char *buf = kmalloc(sizeof(char) * 500);

  int res = 0;

  /* converts from user space to kernal space buffer */
  res = copyin(a1, buf, nbytes);

  if (res) {
    *err = res;
    return -1;
  }

  struct iovec iov;
  struct uio myuio;

  /* initialising uio struct to use VOP functions */
  uio_kinit(&iov, &myuio, buf, nbytes, fp, UIO_WRITE);

  /* check whether each struct created is not null */

  struct vnode *vptr;
  off_t result;

  vptr = _of_vnode(of_ind);

  result = VOP_WRITE(vptr, &myuio);

  if (result) {
    
    *err = result;
    return -1;
  }

  result = _of_progress(of_ind, nbytes);

  if (result) {
    *err = result;
    return -1;
  }

  kfree(buf);

  return nbytes;

}

ssize_t
sys_read(int fd, userptr_t buf, size_t buflen, int *err)
{

  int of_ind;

  of_ind = _fd_point(fd, err);

  if (*err) {
    /* invalid fd */
    return -1;
  }

  off_t fp;
  
  fp = _of_flag(of_ind, O_RDONLY, err);

  if (*err) {
    /* insufficient user permissions */
    return -1;
  }
  
  struct iovec iov;
  struct uio myuio;

  /* initialising uio struct to use VOP functions */
  uio_uinit(&iov, &myuio, buf, buflen, fp, UIO_READ);

  struct vnode *vptr;
  off_t result;

  vptr = _of_vnode(of_ind);

  result = VOP_READ(vptr, &myuio);

  if (result) {
    *err = result;
    return -1;
  }

  result = _of_progress(of_ind, myuio.uio_offset);

  if (result) {
    *err = result;
    return -1;
  }

  return myuio.uio_offset - fp;

}

int
sys_lseek(int fd, uint32_t hi, uint32_t lo, userptr_t poi, uint32_t *v0, uint32_t *v1)
{

  uint64_t pos;
  int whence;

  /* converting 2 32 bit arguments into single 64 bit pos */
  join32to64(hi, lo, &pos);

  /* retrieving stack pointer argument  */
  copyin((userptr_t)poi, &whence, sizeof(int));

  int of_ind;
  int result = 0;

  of_ind = _fd_point(fd, &result);
  
  if (result) {
    /* invalid fd */
    return result;
  }

  off_t fp;
  
  fp = _of_flag(of_ind, O_RDWR, &result);

  if (result) {
    /* insufficient user permissions */
    return result;
  }

  if (whence == SEEK_SET) {
    fp = pos;
  } else if (whence == SEEK_CUR) {
    fp += pos;
  } else if (whence == SEEK_END) {

    struct stat *statbuf = kmalloc(sizeof(statbuf));
    struct vnode *vptr;
    
    vptr = _of_vnode(of_ind);

    result = VOP_STAT(vptr, statbuf);

    if (result) {
      return result;
    }

    fp = statbuf->st_size + pos;

    kfree(statbuf);

  } else {
    return EINVAL;
  }

  if (fp < 0) {
    return EINVAL;
  }

  result = _of_set_fp(of_ind, fp);

  if (result) {
    return result;
  }

  split64to32(fp, v0, v1);

  return 0;
}

int
sys_dup2(int newfd, int oldfd, uint32_t *v0)
{

  int old_of_ind;
  int result = 0;

  /* finds the of index oldfd points to */
  old_of_ind = _fd_point(oldfd, &result);

  if (result) {
    return result;
  }

  int new_of_ind;

  new_of_ind = _fd_point(newfd, &result);

  if (result && new_of_ind == -1) {
    return result;
  } else if (!result && new_of_ind != 0) {
    
    result = 0;

    result = _fd_close(newfd);

    if (result) {
      return result;
    }

  }

  newfd = _fd_allocate(old_of_ind, &result);

  if (result) {
    return EMFILE;
  }

  *v0 = newfd;

  return 0;

}