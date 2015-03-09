/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <copyinout.h>
#include <thread.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <kern/filesys.h>
#include <kern/process.h>
/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, int argc, char ** argv)
{	//size_t len;
	
	//char *kprogram=kmalloc(PATH_MAX*sizeof(char));
	//if(kprogram==NULL){*retval=-1;return ENOMEM; }
	
	//int err=copyinstr((userptr_t)progname, kprogram,PATH_MAX,&len);
	//if(err) {kfree(kprogram);*retval=-1;return err; }
	//if(len==1){ kfree(kprogram);*retval=-1;return EINVAL;}

	int a[argc];
	int off[argc+1];
	off[0]=4*(argc+1);
	for (int i=0;i<argc;i++){
		int k=strlen(argv[i])+1;
		int y=(k%4);
		if (y==0) a[i]=k;
		else a[i]=k+4-y;
		off[i+1]=off[i]+a[i];
	}

	int stringlen=off[argc];
	char *karg=kmalloc(stringlen);
	if(karg==NULL) {
	//kfree(kprogram);
	return ENOMEM;}
	bzero(karg,stringlen);
	
	//size_t g;
	for(int n=0;n<argc;n++){
		memcpy(karg+off[n],argv[n],a[n]);
		//err=copyinstr((userptr_t)argv[n],(karg+off[n]),a[n],&g);
		//if(err) {
		//	kfree(karg);
			//kfree(kbuff);
		//	kfree(kprogram);
		//	return err;
			}
	
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	int err= fdesc_init();
	if (err){
	kfree(karg);
	 return err;
	}
	//struct process *proc=kmalloc(sizeof(struct process));
		
	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		kfree(karg);
		return result;
	}

	/* We should be a new thread. */
	KASSERT(curthread->t_addrspace == NULL);

	/* Create a new address space. */
	curthread->t_addrspace = as_create();
	if (curthread->t_addrspace==NULL) {
		kfree(karg);
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_addrspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		kfree(karg);
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_addrspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		kfree(karg);
		return result;
	}
	/*pid_t k=pid_alloc();
	curthread->t_pid=k;
	err=process_create(k,curthread);
	if (err) {
		return err;
		}*/	
	
	stackptr=stackptr-stringlen;
	char ** kptr=kmalloc(4*(argc+1));
	if(kptr==NULL) return ENOMEM;
	for (int i=0;i<argc;i++){
	kptr[i]=(char *)(stackptr+off[i]);
	}
	kptr[argc]=NULL;
	memcpy(karg, kptr, 4*(argc+1));

	err=copyout(karg,(userptr_t)stackptr,stringlen);
	if (err){ 
		kfree(kptr);
		//kfree(kbuff);
		//kfree(kprogram);
		kfree(karg);
		return err;
		}
	curthread->t_name=kstrdup(progname);
	/* Warp to user mode. */
	enter_new_process(argc, (userptr_t)stackptr,
			  stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

