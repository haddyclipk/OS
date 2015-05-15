#ifndef _PROCESS_H_
#define _PROCESS_H_
#include <types.h>
#include <limits.h>
#include <thread.h>
#include <synch.h>
extern struct process* ptable[130];

struct process
{ pid_t ppid;
bool exited;
int exitcode;
struct thread *self;
struct semaphore *sem_proc;
//struct lock *lock_proc;
};

pid_t pid_alloc(void);
int process_create(pid_t pid, struct thread *thread);
void process_destroy(pid_t pid);

#endif /* _PROCESS_H_ */
