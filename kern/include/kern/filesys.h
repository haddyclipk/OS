#ifndef FILESYS_H_ 
#define FILESYS_H_ 
#include <types.h>
#include <limits.h>
#include <thread.h>
#include <synch.h>

 /* File Descriptor Structure */
 struct fd{
	char filename[__NAME_MAX];
        struct vnode *vn; //   - Reference to the underlying file 'object' 
        off_t offset;     //      - Offset into the file 
        struct lock *lock;   //- Some form of synchronization 
        int flags;      // - Flags with which the file was opened 
        int ref_count; //- Reference count 
 };

 
 int sys_open(const char *fdesc_name, int flags, mode_t mode, int *retval);
 int sys_close(int fh, int *retval);
 int sys_write(int fd, const void *buf, size_t size, int *retval);
 int sys_read(int fd, void *buf, size_t buflen, int *retval);
 int sys_lseek(int fd, off_t pos, int whence, off_t *ret);
 int sys_dup2(int oldfd, int newfd, int *retval);
 int sys_chdir(const char *pathname, int *retval);
 int sys___getcwd(char *buf, size_t buflen, int *retval);

int fdesc_init(void);

 #endif /* FILESYS_H_ */

