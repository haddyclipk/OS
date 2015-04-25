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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */
	as->ptable=kmalloc(sizeof(struct PTE));
	as->region=kmalloc(sizeof(struct region));
	as->heap_base=0;
	as->heap_top=0;
	as->stack_base=0;
	as->stack_top=0;

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL || old == NULL) {
		return ENOMEM;
	}

	//             added by junmo
	newas->heap_base = old->heap_base;
	newas->heap_top = old->heap_top;
	newas->stack_base = old->stack_base;
	newas->stack_top = old->stack_top;
	//
//	if(old->ptable == NULL) return ENOMEM;
//		newas->ptable->pa = old->ptable->pa;
//		newas->ptable->va = old->ptable->va;
//
//	if(old->region == NULL) return ENOMEM;
//		newas->region = old->region;
	//
	struct PTE * temp1=old->ptable;
	struct PTE * temp2=newas->ptable;

	struct region * temp3 = old->region;
	struct region * temp4 = newas->region;

	while(old->ptable->next != NULL){
		newas->ptable->next = kmalloc(sizeof(struct PTE));

		newas->ptable->pa = old->ptable->pa;
		newas->ptable->va = old->ptable->va;
		newas->ptable = newas->ptable->next;
		old->ptable = old->ptable->next;
	}
	old->ptable = temp1;
	newas->ptable = temp2;

	while(old->region->next != NULL){
		newas->region->next = kmalloc(sizeof(struct region));

		newas->region->psize = old->region->psize;
		newas->region->vbase = old->region->vbase;
		newas->region = newas->region->next;
		old->region = old->region->next;
	}
	old->region = temp3;
	newas->region = temp4;
	/*
	 * Write this.
	 */

	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	as->heap_base = 0;
	as->heap_top = 0;
	as->stack_base = 0;
	as->stack_top = 0;

	struct PTE * tmp1 = NULL;
	struct PTE * temp1 = NULL;
	struct region * tmp2 = NULL;
	struct region * temp2 = NULL

	while(as->ptable != NULL){
		tmp1 = as->ptable;
		as->ptable->pa = 0;
		as->ptable->va = 0;
		as->ptable = as->ptable->next;
		temp1=tmp1;
		kfree(temp1);
	}

	while(as->region!= NULL){
		tmp2 = as->region;
		as->region->psize = 0;
		as->region->vbase = 0;
		as->region = as->region->next;
		temp2 = tmp2;
		kfree(temp2);
	}
	
	kfree(as);
}

void
as_activate(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;  // suppress warning until code gets written
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */
	while(as->region->next != NULL)

	(void)as;
	(void)vaddr;
	(void)sz;
	(void)readable;
	(void)writeable;
	(void)executable;
	return EUNIMP;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
}

