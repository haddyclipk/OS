#include <syscall.h> 
#include <lib.h> 
#include <vfs.h> 
#include <vnode.h> 
#include <stdarg.h> 
#include <types.h> 
#include <kern/errno.h> 
#include <kern/fcntl.h> 
#include <limits.h> 
#include <uio.h> 
#include <kern/iovec.h> 
#include <synch.h> 
#include <current.h> 
#include <copyinout.h> 
#include <kern/seek.h> 
#include <kern/stat.h> 
#include <kern/filesys.h> 

int fdesc_init(void) { 
	int result1;
	struct vnode *v1;
	curthread->fdtable[0] = (struct fd *)kmalloc(sizeof(struct fd));
	if (curthread->fdtable[0]==NULL) {return ENOMEM;}
	char fname1[5]="con:";
	result1 = vfs_open(fname1, O_RDONLY, 0664, &v1);

	if(result1){
		kfree(curthread->fdtable[0]);
		return EINVAL;
	}
	
	curthread->fdtable[0]->vn=v1;
	strcpy(curthread->fdtable[0]->filename,fname1);
	curthread->fdtable[0]->ref_count=1;
	curthread->fdtable[0]->flags = O_RDONLY;
	curthread->fdtable[0]->offset = 0;   
	curthread->fdtable[0]->lock = lock_create("STDIN");
	if (curthread->fdtable[0]->lock==NULL) return ENOMEM;

	int result2;
	struct vnode *v2;
	curthread->fdtable[1] = (struct fd *)kmalloc(sizeof(struct fd));

	if (curthread->fdtable[1] ==NULL) {return ENOMEM;}
	char fname2[5]="con:";
	result2 = vfs_open(fname2, O_WRONLY, 0664, &v2);

	if(result2){
		kfree(curthread->fdtable[1] );
		return EINVAL;
	}
	
	curthread->fdtable[1] ->vn=v2;
	strcpy(curthread->fdtable[1]->filename,fname2);
	curthread->fdtable[1] ->ref_count=1;
	curthread->fdtable[1] ->flags = O_WRONLY;
	curthread->fdtable[1] ->offset = 0;   
	curthread->fdtable[1] ->lock = lock_create("STDOUT");
	if (curthread->fdtable[1] ->lock==NULL) return ENOMEM; 

	int result3;
	struct vnode *v3;

	curthread->fdtable[2]  = (struct fd *)kmalloc(sizeof(struct fd));

	if (curthread->fdtable[2]==NULL) {return ENOMEM;}
	char fname3[5]="con:";
	result3 = vfs_open(fname3, O_WRONLY, 0664, &v3);

	if(result3){
		kfree(curthread->fdtable[2]);
		return EINVAL;
	}
	
	curthread->fdtable[2]->vn=v3;
	strcpy(curthread->fdtable[2]->filename,fname3);
	curthread->fdtable[2]->ref_count=1;
	curthread->fdtable[2]->flags = O_WRONLY;
	curthread->fdtable[2]->offset = 0;   
	curthread->fdtable[2]->lock = lock_create("STDERR");
	if (curthread->fdtable[2]->lock==NULL) return ENOMEM;
	
	return 0; 
}  

int sys_open(const char *fdesc_name, int flags, mode_t mode , int *retval) { 

 	int result=0, index = 3; 
	struct vnode *vn; 
 	char *fname; 
	size_t len;
        fname = (char *)kmalloc(sizeof(char)*PATH_MAX);
         if(copyinstr((const_userptr_t)fdesc_name, fname, PATH_MAX, &len)){
                kfree(fname);
		*retval = -1;
                return EFAULT;
        }

	while(curthread->fdtable[index] != NULL){
		if(index <OPEN_MAX) index++;
	}
 
 	if(index == OPEN_MAX) { 
		kfree(fname);
		
 		return EMFILE;  //fdtable[] is fulled no more valid place.
 	} 
 	curthread->fdtable[index] = (struct fd*)kmalloc(sizeof(struct fd)); 
	
	result = vfs_open(fname,flags,mode,&vn); 
	if(result) { 
 		kfree(curthread->fdtable[index]); 
		kfree(fname);
		
 		curthread->fdtable[index] = NULL; 
 		return result; 
 	} 
  	strcpy(curthread->fdtable[index]->filename,fname);
	curthread->fdtable[index]->lock= lock_create(fname); 
	
	kfree(fname);
	if(curthread->fdtable[index]->lock == NULL) return ENOMEM;
 	curthread->fdtable[index]->vn = vn; 
 	curthread->fdtable[index]->flags = flags; 
 	curthread->fdtable[index]->ref_count = 1; 
 	curthread->fdtable[index]->offset = 0; 
 	
	
  	
	
	*retval = index;
 	return 0; 
 } 
 
 
int sys_close(int fh, int *retval) { 

 	if(fh >= OPEN_MAX ){
		*retval = -1;
		return EBADF;
	}

	if(fh < 0) { 
		*retval = -1;
 		return EBADF;  //not valid location in fdtable[]
 	} 

 	if(curthread->fdtable[fh] == NULL) { 
		*retval = -1;
 		return EBADF;  // already empty or closed so that fdtable[fh] is closed
 	}  
 
 	if(curthread->fdtable[fh]->vn == NULL) { 
		*retval = -1;
 		return EBADF;  // if there is no valid fdesc inside of fdtable
 	} 
  	
 	if(curthread->fdtable[fh]->ref_count == 1) {  
 		vfs_close(curthread->fdtable[fh]->vn);
		curthread->fdtable[fh]->ref_count =0;
                lock_destroy(curthread->fdtable[fh]->lock);
		kfree(curthread->fdtable[fh]); 
 		curthread->fdtable[fh] = NULL;
 	}else curthread->fdtable[fh]->ref_count--;
   	int *k=kmalloc(sizeof(int));
	kfree(k);
	*retval = 0;
 	return 0;  
} 

int sys_read(int fd, void *buf, size_t buflen, int *retval){
	if(fd >= OPEN_MAX) {
		*retval = -1;
		return EBADF;
	}
	if(fd < 0) {
		*retval = -1;
		return EBADF;
	}
	if(curthread->fdtable[fd] == NULL) {
		*retval = -1;
		return EBADF;
	}
	if(curthread->fdtable[fd]->flags == O_WRONLY) {
		*retval = -1;
		return EBADF;
	}
	struct uio u;
	struct iovec i;
	
	lock_acquire(curthread->fdtable[fd]->lock);

	i.iov_ubase = (userptr_t)buf;
	i.iov_len = buflen;
	u.uio_iov = &i;
	u.uio_iovcnt = 1;
	u.uio_offset = curthread->fdtable[fd]->offset;
	u.uio_resid = buflen;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curthread->t_addrspace;

	int result = VOP_READ(curthread->fdtable[fd]->vn, &u);
	if(result){
		lock_release(curthread->fdtable[fd]->lock);
		
		return result;
	}
	curthread->fdtable[fd]->offset = u.uio_offset;
	*retval = buflen - u.uio_resid;
	lock_release(curthread->fdtable[fd]->lock);
	
	return 0;
}

int sys_write(int fd, const void *buf, size_t buflen, int *retval){

	if(fd >= OPEN_MAX) {
		*retval = -1;
		return EBADF;
	}
	if(fd < 0) {
		*retval = -1;
		return EBADF;
	}
	if(curthread->fdtable[fd] == NULL) {
		*retval = -1;
		return EBADF;
	}
	if(!buf){
		*retval = -1;
		 return EFAULT;
	}

	if(curthread->fdtable[fd]->flags == O_RDONLY) {
		*retval = -1;
		return EBADF;
	}
	struct uio u;
	struct iovec i;
	
	lock_acquire(curthread->fdtable[fd]->lock);

	i.iov_ubase = (userptr_t)buf;
	i.iov_len = buflen;
	u.uio_iov = &i;
	u.uio_iovcnt = 1;
	u.uio_offset = curthread->fdtable[fd]->offset;
	u.uio_resid = buflen;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = curthread->t_addrspace;

	int result = VOP_WRITE(curthread->fdtable[fd]->vn, &u);
	if(result){
		lock_release(curthread->fdtable[fd]->lock);
		
		return result;
	}
	curthread->fdtable[fd]->offset = u.uio_offset;
	*retval = buflen - u.uio_resid;
	lock_release(curthread->fdtable[fd]->lock);

	return 0;
}

int sys_dup2(int oldfd, int newfd, int *retval){
	int result = 0;
	if(oldfd >= OPEN_MAX){ 
		*retval = -1;
		 return EBADF;
	}
	if(oldfd<0){
		*retval = -1;
		return EBADF;
	}
	if(newfd>=OPEN_MAX){
		*retval = -1;
		return EBADF;
	}
	if(newfd<0){
		*retval = -1;
		return EBADF;
	}
	if(newfd == oldfd){
		*retval = newfd;
		return 0;
	}
	if(curthread->fdtable[oldfd] == NULL) {
		*retval = -1;
		return EBADF;
	}
	if(curthread->fdtable[newfd] != NULL){ 
		result = sys_close(newfd,retval);
		if(result){ 
			*retval = -1;
			return EBADF;
		}
	}

	curthread->fdtable[newfd] = curthread->fdtable[oldfd];

	*retval = newfd;
	return 0;
}

int sys_lseek(int fd, off_t pos, int whence, off_t *retval){
	if(fd >= OPEN_MAX){
		 *retval = (off_t)-1;
		 return EBADF;
	}
	if(fd < 0) {
		*retval = (off_t)-1;
		return EBADF;
	}
	if(curthread->fdtable[fd] == NULL) {
		*retval = (off_t)-1;
		return EBADF;
	}
	
	struct stat s; 
	off_t noffset, size, curr, pos1;
	int result = 0;
	
	lock_acquire(curthread->fdtable[fd]->lock);	
	
	if(VOP_STAT(curthread->fdtable[fd]->vn, &s)){
		lock_release(curthread->fdtable[fd]->lock);
		*retval = (off_t)-1;
		return EBADF;
	}
	size = s.st_size;
	pos1 = pos;
	result = strcmp(curthread->fdtable[fd]->filename, "null");
	if(result == 0){
		lock_release(curthread->fdtable[fd]->lock);
		*retval = (off_t)-1;
		return ESPIPE;
	}
	if(whence == SEEK_SET){
		result = VOP_TRYSEEK(curthread->fdtable[fd]->vn, (pos1));
		if(result){
			lock_release(curthread->fdtable[fd]->lock);	
			*retval =(off_t) -1;
			return EINVAL;
		}
		noffset = pos1;
	}else if(whence == SEEK_CUR){
		result = VOP_TRYSEEK(curthread->fdtable[fd]->vn, (pos1+curr));
		if(result){
			lock_release(curthread->fdtable[fd]->lock);	
			*retval =(off_t) -1;
			return ESPIPE;
		}
		curr = curthread->fdtable[fd]->offset;
		noffset = pos1+curr;
	}else if(whence == SEEK_END){
		result = VOP_TRYSEEK(curthread->fdtable[fd]->vn, (pos + size));
		if(result){
			lock_release(curthread->fdtable[fd]->lock);		
			*retval =(off_t) -1;
			return EINVAL;
		}
		noffset = pos1 + size;
	}else{
		lock_release(curthread->fdtable[fd]->lock);
		*retval = (off_t)-1;
		return EINVAL;
	}
	curthread->fdtable[fd]->offset = noffset;
	
	*retval = noffset;
	lock_release(curthread->fdtable[fd]->lock);
	return 0;
}

int sys_chdir(const char *pathname, int *retval){
	char *path;
	size_t len;
	path = (char *)kmalloc(sizeof(char)*PATH_MAX);
	int result = 0;
	result = copyinstr((const_userptr_t)pathname, path, PATH_MAX, &len);
	if(result){
		kfree(path);
		*retval = -1;		
		return EFAULT;
	}
	result = vfs_chdir(path);
	if(result) { 
		kfree(path);
		
		return ENOTDIR;
	}
	*retval = 0;
	kfree(path);
	return 0;
}

int sys___getcwd(char *buf, size_t buflen, int *retval){
	struct uio u;
	struct iovec i;
	if(buf==NULL) return EFAULT;
	
	//char *nbuf=(char *)kmalloc(sizeof(char)*buflen);	
	int result = 0;
	//if(copyinstr((const_userptr_t)buf, nbuf, PATH_MAX, &buflen)){
	//	 *retval = -1;
	//	 return EFAULT;
	//}

	i.iov_ubase = (userptr_t)buf;
	i.iov_len = (buflen -1);
	u.uio_iov = &i;
	u.uio_iovcnt = 1;
	u.uio_offset = (off_t)0;
	u.uio_resid = (buflen-1);
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curthread->t_addrspace;
	
	
	result = vfs_getcwd(&u);
	if(result) {
		
		return ENOENT;
	}
	int re = strlen(buf);
		
	*retval = re;
	return 0;
}
