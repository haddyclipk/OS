#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <array.h>
#include <cpu.h>
#include <spl.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <threadlist.h>
#include <threadprivate.h>
#include <current.h>
#include <synch.h>
#include <addrspace.h>
#include <mainbus.h>
#include <vnode.h>
#include <kern/process.h>
#include <kern/filesys.h>

 //void thread_destroy(struct thread *thread);
struct process *ptable[256];

pid_t pid_alloc(void){
	
	for (int i=2; i<256; i++){
		if (ptable[i]==NULL){
		return (pid_t)i;
		}
	}
	return (pid_t)-2; 
}

int  process_create(pid_t pid, struct thread *thread){
		struct process *proc=kmalloc(sizeof(struct process));
		if(proc==NULL)
			return ENOMEM;
		proc->ppid=-1;
		proc->exited=false;
		proc->exitcode=0;
		proc->self=thread;
		proc->sem_proc=sem_create("sema_process",0);
		if (proc->sem_proc==NULL) {
			 kfree(proc);
			return ENOMEM;
		}
		//proc->lock_proc=lock_create("lock_process");
		//if (proc->lock_proc==NULL) {
		//	cv_destroy(proc->cv_proc); 
		//	kfree(proc);
		//	return ENOMEM;
		//}
		ptable[pid]=proc;
		return 0;
}

void process_destroy(pid_t pid){
	//lock_destroy(ptable[pid]->lock_proc);
	sem_destroy(ptable[pid]->sem_proc);
	
	//int ret=0;
	//int k=0;	
	//for(int a=0;a<OPEN_MAX;a++){
	//if (ptable[pid]->self->fdtable[a]!=NULL){
	//k=sys_close(a,&ret);
	//	}
	//}
	
	//kfree(ptable[pid]->self->t_name);
	//kfree(ptable[pid]->self);
	kfree(ptable[pid]);
	ptable[pid]=NULL;
}
