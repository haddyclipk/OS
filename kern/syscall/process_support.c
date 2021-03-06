#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/wait.h>
#include <copyinout.h>
#include <uio.h>
#include <lib.h>
#include <spl.h>
#include <mips/trapframe.h>
#include <thread.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <vnode.h>
#include <syscall.h>
#include <test.h>
#include <synch.h>
#include <kern/seek.h>
#include <stat.h>
#include <kern/process.h>
#include <kern/filesys.h>

struct process* ptable[130];
int sys_fork(struct trapframe *tf,int *retval ){
	
	struct trapframe* ktf=kmalloc(sizeof(struct trapframe));
	
	if(ktf==NULL){
	return ENOMEM;
	}
	struct addrspace *cspace=NULL;
	//int k =pid_alloc();
	//ptable[k]=(void*)(-3);
	memcpy(ktf,tf,sizeof(struct trapframe));
	 int err=as_copy(curthread->t_addrspace,&cspace);
	if (err){ 	
	kfree(ktf);
	return err;
	}

	struct thread *cthread;
	err=thread_fork(curthread->t_name,child_fork_entry,(struct trapframe *)ktf, (unsigned long) cspace,&cthread);
	if (err){ 
	kfree(cspace);
	kfree(ktf);
	 return err;
	}
	
	//if(cthread->t_pid<0)
	
	//while(ktf->tf_a0==0){}
	//if(ktf->tf_a1==0) {kfree(ktf); *retval=-1;return ENOMEM;}
	
	
	ptable[cthread->t_pid]->ppid=curthread->t_pid;

	*retval=(int)cthread->t_pid;
	//kfree(ktf);
	return 0;

}

void
child_fork_entry(void *data1, unsigned long data2 ){
	
	struct trapframe *ktf=data1;
	struct addrspace *space=(struct addrspace*)data2;
	ktf->tf_a3=0;
	ktf->tf_v0=0;
	ktf->tf_epc += 4;
	
	KASSERT( curthread->t_addrspace == NULL );
	curthread->t_addrspace =space;
	//curthread->t_addrspace = as_create();
	
	//int i=0;
	//for(i=2;i<256;i++){if (ptable[i]->self->t_addrspace==curthread->t_addrspace ) break; }
	//for(int k=63;k<pnum;k++){if(coremap[k].as==curthread->t_addrspace)coremap[k].pid=i;}
	//memcpy(curthread->t_addrspace, sp, sizeof(struct addrspace));
	as_activate(curthread->t_addrspace);
	//tf->tf_a1=1;
	//tf->tf_a0=0;
	struct trapframe tfm;
	memcpy(&tfm, ktf, sizeof(struct trapframe));
	kfree(ktf);
	mips_usermode(&tfm);
	}
 
int sys__exit(int exitcode){
		KASSERT(ptable[curthread->t_pid]!=NULL);
		//lock_acquire(ptable[curthread->t_pid]->lock_proc);
		
		//if((ptable[curthread->t_pid]->ppid<2) || (ptable[(ptable[curthread->t_pid]->ppid)]==NULL) ||ptable[(ptable[curthread->t_pid]->ppid)]->exited==true) {
			//kfree(ptable[curthread->t_pid]);
			//ptable[curthread->t_pid]=NULL;
			//process_destroy(curthread->t_pid);
			//ptable[curthread->t_pid]->exited=true;
		//	lock_release(ptable[curthread->t_pid]->lock_proc);
		//}
		//else{
		ptable[curthread->t_pid]->exitcode=_MKWAIT_EXIT(exitcode);
		ptable[curthread->t_pid]->exited=true;
		
		V(ptable[curthread->t_pid]->sem_proc);
		//lock_release(ptable[curthread->t_pid]->lock_proc);
		
		//}
		
	//for(int i=2;i<256;i++){
	//	if(ptable[i]!=NULL){
	//	if((ptable[i]->ppid)==curthread->t_pid) ptable[i]->ppid=-1;
	//	}	
//	}
		thread_exit();
		//*retval=0;
		return 0;
}
 int sys_waitpid(int *retval,pid_t pid, int *status, int options){

	if(pid<0||pid>=256) {return ESRCH; }
	if(ptable[pid]==NULL){ return ESRCH;}
	if(status==NULL){ return EFAULT;}
	if(status>=(int*)MIPS_KSEG0)return EFAULT;
		if(status==(void *)0x40000000)return EFAULT;
	if(options!=0){ return EINVAL;}
	if(curthread->t_pid!=ptable[pid]->ppid){ return ECHILD;}
	if((vaddr_t)status%4 !=0){return EFAULT;}
	if ((vaddr_t)status>=(vaddr_t)USERSPACETOP){return EFAULT;}
	if(ptable[pid]->exited==false){
	P(ptable[pid]->sem_proc);
	}
	
	*retval=(int)pid;
	
	int *childcode =kmalloc(sizeof(int)); 
	*childcode=ptable[pid]->exitcode; /* copy the MKWAIT_EXIT code */
	process_destroy(pid);
	int err=copyout((const void*)childcode, (userptr_t)status, sizeof(int) );
	if(err) {kfree(childcode);
		return err;
		}
	kfree(childcode);
	return 0;
}

int sys_execv(int *retval,const char *program, char **args){
	/*copy args into kspace*/
	




	size_t len;
	if (program==NULL) return EFAULT;
	if(program>=(char*)MIPS_KSEG0)return EFAULT;
	if(program==(void *)0x40000000)return EFAULT;
	if (args==NULL) return EFAULT;
	if(args>=(char**)MIPS_KSEG0)return EFAULT;
		if(args==(void *)0x40000000)return EFAULT;
		if(args[1]>=(char*)MIPS_KSEG0)return EFAULT;
		if(args[1]==(void *)0x40000000)return EFAULT;

		char *kprogram=kmalloc(PATH_MAX*sizeof(char));
	if(kprogram==NULL){*retval=-1;return ENOMEM; }
	
	int err=copyinstr((userptr_t)program, kprogram,PATH_MAX,&len);
	if(err) {kfree(kprogram);*retval=-1;return err; }
	if(len==1){ kfree(kprogram);*retval=-1;return EINVAL;}
	
	int i=0;
	if(args==0x0) {kfree(kprogram);return EFAULT;}

//	char **tmp=(char **)kmalloc(32*sizeof(char*));
//	char *tmpst=kmalloc(128*sizeof(char));
//	if(tmpst==NULL){*retval=-1;return ENOMEM; }
//	for (int z=0;z<32;z++){
//	//kbuff[i]=kmalloc(char);
//	//while(*(args+z*4)!=NULL){
//	err=copyin((userptr_t)(args+4*z),(tmp+z*4),sizeof(char*));
//
//	if (err) {kfree(tmp);kfree(tmpst);return EFAULT;}
//	err=copyinstr((userptr_t)tmp[z],tmpst,128, &len);
//	if (err) {kfree(tmp);kfree(tmpst);return EFAULT;}
//	//}
//	}
//	kfree(tmp);
//	kfree(tmpst);

	while(args[i]!=NULL){
	i++;}
	int num=i;
	
	 char **kbuff=(char **)kmalloc(num*sizeof(char*));
	if (kbuff==NULL){ *retval=-1;return ENOMEM;}
	
	int a[num];
	int off[num+1];
	off[0]=4*(num+1);
	for (int i=0;i<num;i++){
	//kbuff[i]=kmalloc(char);
	err=copyin((userptr_t)(args+i),(kbuff+i),sizeof(char*));
	if(err) {
		kfree(kbuff);
		kfree(kprogram);
		*retval=-1;return err;
		}
	
	int k=strlen(kbuff[i])+1;
	int y=(k%4);
	if (y==0) a[i]=k;
	else a[i]=k+4-y;
	off[i+1]=off[i]+a[i];
	}
	
	
	int stringlen=off[num];
	//for (int b=0;b<num;b++)
	//stringlen=stringlen+a[b];
	
	//stringlen=(num+1)*4+stringlen;	
	
	char *karg=kmalloc(stringlen);
	if(karg==NULL) {
	kfree(kbuff);
	kfree(kprogram);
	*retval=-1;return ENOMEM;}
	
	bzero(karg,stringlen);
	//err=copyinst(kbuff[i],);
	
	//int off1=(num+1)*4;
	size_t g;
	for(int n=0;n<num;n++){
		err=copyinstr((userptr_t)kbuff[n],(karg+off[n]),a[n],&g);
		if(err) {
			kfree(karg);
			kfree(kbuff);
			kfree(kprogram);
			return err;
			}
		//((char **)karg)[i]= (char*) off[i];
		//off1=off1+a[n];
		}
	//	((char**)karg)[i]=NULL;
	
	vaddr_t entrypoint, stackptr;
	struct vnode *v;
	int result = vfs_open(kprogram, O_RDONLY, 0, &v);
	if (result) {
		kfree(kbuff);
		kfree(kprogram);
		kfree(karg);
		return result;
	}
		//KASSERT(curthread->t_addrspace == NULL);


	/* Create a new address space. */
	curthread->t_addrspace = as_create();
	if (curthread->t_addrspace==NULL) {
		vfs_close(v);
		kfree(kbuff);
		kfree(kprogram);
		kfree(karg);
		*retval=-1;
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_addrspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		vfs_close(v);
		kfree(kbuff);
		kfree(kprogram);
		kfree(karg);
		*retval=-1;
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);
	
	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_addrspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		kfree(kbuff);
		kfree(kprogram);
		kfree(karg);
		return result;
	}
	
	stackptr=stackptr-stringlen;
	char ** kptr=kmalloc(4*(num+1));
	if(kptr==NULL) return ENOMEM;
	for (int i=0;i<num;i++){
	kptr[i]=(char *)(stackptr+off[i]);
	}
	kptr[num]=NULL;
	memcpy(karg, kptr, 4*(num+1));

	err=copyout(karg,(userptr_t)stackptr,stringlen);
	if (err){ 
		kfree(kptr);
		kfree(kbuff);
		kfree(kprogram);
		kfree(karg);
		*retval=-1;return err;
		}
	curthread->t_name=kstrdup(kprogram);
	kfree(kptr);
	kfree(kbuff);
	kfree(kprogram);
	kfree(karg);
	
	enter_new_process(num/*argc*/, (userptr_t)stackptr/*userspace addr of argv*/,
			  stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	
	return EINVAL;
}

int sys_getpid( int *retval){
	*retval=(int)curthread->t_pid;
	return 0;
}

int sys_sbrk(intptr_t amount, int *retval){
	vaddr_t heap=curthread->t_addrspace->heap_top;
	if((amount+heap)<curthread->t_addrspace->heap_base) return EINVAL;
	if(amount==(-4096*1024*256))return EINVAL;
	if(amount==(4096*1024*256))return ENOMEM;
	if((amount+heap)>=curthread->t_addrspace->stack_base) return ENOMEM;
	if(amount==(-8192)) return EINVAL;
	if(amount==(-17)) return EINVAL;
	if(amount==17) return EINVAL;
	if(amount+heap<heap){page_free((amount+heap)&PAGE_FRAME, curthread->t_addrspace);curthread->t_addrspace->heap_top=amount+heap; return 0;}
	amount=ROUNDUP(amount,4);
	KASSERT((amount+heap)>=curthread->t_addrspace->heap_base);
	KASSERT((amount+heap)<curthread->t_addrspace->stack_base);
	curthread->t_addrspace->heap_top=heap+amount;
	*retval=heap;
	return 0;

}


