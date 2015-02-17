/*
 * Copyright (c) 2001, 2002, 2009
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
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * 08 Feb 2012 : GWA : Driver code is in kern/synchprobs/driver.c. We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

static struct lock *male_lock;
static struct lock *female_lock;
static struct lock *matchmaker_lock;

static struct semaphore *count1;
static struct semaphore *count2;
static struct semaphore *count3;

void whalemating_init() {
  	
	
	male_lock = lock_create("male_lock");
  	DEBUGASSERT(male_lock != NULL);
	female_lock = lock_create("female_lock");
  	DEBUGASSERT(female_lock != NULL);
	matchmaker_lock = lock_create("matchmaker_lock");
  	DEBUGASSERT(matchmaker_lock != NULL);
	count1 = sem_create("count1",0);
  	DEBUGASSERT(count1 != NULL);	
	count2 = sem_create("count2",0);
  	DEBUGASSERT(count2 != NULL);	
	count3 = sem_create("count3",0);
  	DEBUGASSERT(count3 != NULL);	
//return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void whalemating_cleanup() {
sem_destroy(count3);
sem_destroy(count2);
sem_destroy(count1);

lock_destroy(matchmaker_lock);  
lock_destroy(female_lock);  
lock_destroy(male_lock);  


//return;
}

void
male(void *p, unsigned long which)
{	
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
male_start();
	
	lock_acquire(male_lock);
	
	V(count1);
	P(count2);
	
	
	
	
	
	lock_release(male_lock);		
// Implement this function 
  male_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
female(void *p, unsigned long which)
{	
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;

  female_start();
	lock_acquire(female_lock);
	
	
	V(count2);
	P(count3);
	
	
	
	lock_release(female_lock);	
	
// Implement this function 
  female_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
matchmaker(void *p, unsigned long which)
{	
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  matchmaker_start();
	
	lock_acquire(matchmaker_lock);
	
	V(count3);
	P(count1);
	
	
	
	
	
	lock_release(matchmaker_lock);
	// Implement this function 
  matchmaker_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

/*
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is,
 * of course, stable under rotation)
 *
 *   | 0 |
 * --     --
 *    0 1
 * 3       1
 *    3 2
 * --     --
 *   | 2 | 
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X
 * first.
 *
 * You will probably want to write some helper functions to assist
 * with the mappings. Modular arithmetic can help, e.g. a car passing
 * straight through the intersection entering from direction X will leave to
 * direction (X + 2) % 4 and pass through quadrants X and (X + 3) % 4.
 * Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in drivers.c.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.
static struct lock *whole_lock;
static struct lock *lock_0;
static struct lock *lock_1;
static struct lock *lock_2;
static struct lock *lock_3;
static struct semaphore *sem;

void stoplight_init() {
	whole_lock = lock_create("whole_lock");
	DEBUGASSERT(whole_lock != NULL);
	lock_0 = lock_create("lock_0");
  	DEBUGASSERT(lock_0 != NULL);
	lock_1 = lock_create("lock_1");
  	DEBUGASSERT(lock_1 != NULL);
	lock_2 = lock_create("lock_2");
  	DEBUGASSERT(lock_2 != NULL);
	lock_3 = lock_create("lock_3");
  	DEBUGASSERT(lock_3 != NULL);
	sem = sem_create("sem",2);
  	DEBUGASSERT(sem != NULL);
  return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void stoplight_cleanup() {
	sem_destroy(sem);
	lock_destroy(lock_3);
	lock_destroy(lock_2);  
	lock_destroy(lock_1);  
	lock_destroy(lock_0);  
	lock_destroy(whole_lock);
  	return;
}

void
gostraight(void *p, unsigned long direction)
{	 int X;
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  (void)direction;
  	int i = 0;
	X=direction;
	lock_acquire(whole_lock);
	switch(X){
		case 0: 
			lock_acquire(lock_0);
			break;
		case 1:
			lock_acquire(lock_1);
			break;
		case 2:
			lock_acquire(lock_2);
			break;
		case 3:
			lock_acquire(lock_3);
			break;
	}
	inQuadrant(	X);
	P(sem);
	if(sem->sem_count>0) {
	lock_release(whole_lock);
	i=1;
	}
	switch((X+3)%4){
		case 0: 
			lock_acquire(lock_0);
			break;
		case 1:
			lock_acquire(lock_1);
			break;
		case 2:
			lock_acquire(lock_2);
			break;
		case 3:
			lock_acquire(lock_3);
			break;
	}
	inQuadrant(	(X+3)%4);
	switch(X){
		case 0: 
			lock_release(lock_0);
			break;
		case 1:
			lock_release(lock_1);
			break;
		case 2:
			lock_release(lock_2);
			break;
		case 3:
			lock_release(lock_3);
			break;
	}	
	leaveIntersection();
	V(sem);
	
	switch((X+3)%4){
		case 0: 
			lock_release(lock_0);
			break;
		case 1:
			lock_release(lock_1);
			break;
		case 2:
			lock_release(lock_2);
			break;
		case 3:
			lock_release(lock_3);
			break;
	}	
	if (i==0)
	lock_release(whole_lock);

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnleft(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  (void)direction;
  	int i =0;
	int X=direction;
	lock_acquire(whole_lock);
	switch(X){
		case 0: 
			lock_acquire(lock_0);
			break;
		case 1:
			lock_acquire(lock_1);
			break;
		case 2:
			lock_acquire(lock_2);
			break;
		case 3:
			lock_acquire(lock_3);
			break;
	}
	inQuadrant(	X);
	P(sem);
	if(sem->sem_count>0) {
	lock_release(whole_lock);
	i=1;
	}
	switch((X+3)%4){
		case 0: 
			lock_acquire(lock_0);
			break;
		case 1:
			lock_acquire(lock_1);
			break;
		case 2:
			lock_acquire(lock_2);
			break;
		case 3:
			lock_acquire(lock_3);
			break;
	}
	inQuadrant(	(X+3)%4);
	switch(X){
		case 0: 
			lock_release(lock_0);
			break;
		case 1:
			lock_release(lock_1);
			break;
		case 2:
			lock_release(lock_2);
			break;
		case 3:
			lock_release(lock_3);
			break;
	}	
	switch((X+2)%4){
		case 0: 
			lock_acquire(lock_0);
			break;
		case 1:
			lock_acquire(lock_1);
			break;
		case 2:
			lock_acquire(lock_2);
			break;
		case 3:
			lock_acquire(lock_3);
			break;
	}
	inQuadrant(	(X+2)%4);
	
	switch((X+3)%4){
		case 0: 
			lock_release(lock_0);
			break;
		case 1:
			lock_release(lock_1);
			break;
		case 2:
			lock_release(lock_2);
			break;
		case 3:
			lock_release(lock_3);
			break;
	}	
	leaveIntersection();
	V(sem);
	

	switch((X+2)%4){
		case 0: 
			lock_release(lock_0);
			break;
		case 1:
			lock_release(lock_1);
			break;
		case 2:
			lock_release(lock_2);
			break;
		case 3:
			lock_release(lock_3);
			break;
	}	
	if (i==0)
	lock_release(whole_lock);

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnright(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  (void)direction;
	int i = 0;
 	int X=direction;
	lock_acquire(whole_lock);
	switch(X){
		case 0: 
			lock_acquire(lock_0);
			break;
		case 1:
			lock_acquire(lock_1);
			break;
		case 2:
			lock_acquire(lock_2);
			break;
		case 3:
			lock_acquire(lock_3);
			break;
	}
	inQuadrant(	X);
	P(sem);
	if(sem->sem_count>0) {
	lock_release(whole_lock);
	i=1;
	}
	leaveIntersection();
	V(sem);
	

	switch(X){
		case 0: 
			lock_release(lock_0);
			break;
		case 1:
			lock_release(lock_1);
			break;
		case 2:
			lock_release(lock_2);
			break;
		case 3:
			lock_release(lock_3);
			break;
	}	
	if (i==0)
	lock_release(whole_lock);
	
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}
