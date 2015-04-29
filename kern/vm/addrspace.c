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
	as->ptable->PTE_P=0;
	as->ptable->read=0;
	as->ptable->write=0;
	as->ptable->exe=0;
	as->ptable->next = NULL;

	as->region=kmalloc(sizeof(struct region));
	as->region->exe=0;
	as->region->read=0;
	as->region->write=0;
	as->region->flag=0;
	as->region->next = NULL;

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
		newas->ptable->read=old->ptable->read;
		newas->ptable->write=old->ptable->write;
		newas->ptable->exe=old->ptable->exe;
		newas->ptable->PTE_P=old->ptable->PTE_P;
		newas->ptable->pa = old->ptable->pa;
		newas->ptable->va = old->ptable->va;
		newas->ptable = newas->ptable->next;
		newas->ptable->next = NULL;
		old->ptable = old->ptable->next;
	}
	old->ptable = temp1;
	newas->ptable = temp2;

	while(old->region->next != NULL){
		newas->region->next = kmalloc(sizeof(struct region));
		newas->region->flag=old->region->flag;
		newas->region->read=old->region->read;
		newas->region->write=old->region->write;
		newas->region->exe=old->region->exe;
		//newas->region->reg_st=old->region->reg_st;
		newas->region->psize = old->region->psize;
		newas->region->vbase = old->region->vbase;
		newas->region = newas->region->next;
		newas->region->next = NULL;
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
	struct region * temp2 = NULL;

	while(as->ptable != NULL){  ////clean pagetable
		tmp1 = as->ptable;
		page_free(as->ptable->va);
		as->ptable->pa = 0;
		as->ptable->va = 0;
		as->ptable->PTE_P=0;
		as->ptable->read=0;
		as->ptable->write=0;
		as->ptable->exe=0;
		as->ptable = as->ptable->next;
		temp1=tmp1;
		kfree(temp1);
	}

	while(as->region!= NULL){  //clean region
		tmp2 = as->region;
		as->region->read=0;
		as->region->write=0;
		as->region->exe=0;
		//as->region->reg_st=0;///state
		as->region->psize = 0;
		as->region->vbase = 0;
		as->region->flag=0;
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
	vm_tlbshootdown_all();
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
	sz += vaddr & ~(vaddr_t)PAGE_FRAME; //Aligning Regions
	vaddr &= PAGE_FRAME;
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME; //length
	//region
	struct region *tmp=as->region;
	while(tmp->next != NULL){
		tmp=tmp->next;
	}
	tmp->vbase=vaddr;
	tmp->psize=sz/PAGE_SIZE;
	tmp->flag=0;
	if(readable==4)tmp->read=1;
	if(writeable==2)tmp->write=1;
	if(executable==1)tmp->exe=1;
	//initial next region
	tmp->next=kmalloc(sizeof(struct region));
	tmp->next->flag=0;
	tmp->next->read=0;
	tmp->next->write=0;
	tmp->next->exe=0;
	tmp->next->next = NULL;


	// update pagetable
	struct PTE *tmp1=as->ptable;
	while(tmp1->next!=NULL){
		tmp1=tmp1->next;
	}

	for(size_t i=0;i<tmp->psize;i++){
	tmp1->va=vaddr+i*PAGE_SIZE;
	//update permission
	tmp1->read=tmp->read;
	tmp1->write=tmp->write;
	tmp1->exe=tmp->exe;

	tmp1->pa=page_alloc();
	tmp1->PTE_P=1;
	//coremap update
	coremap[KVADDR_TO_PADDR(tmp1->pa)/PAGE_SIZE].va=tmp1->va;
	coremap[KVADDR_TO_PADDR(tmp1->pa)/PAGE_SIZE].as=as;
	coremap[KVADDR_TO_PADDR(tmp1->pa)/PAGE_SIZE].pgstate=DIRTY;
	//inital next PTE
	tmp1->next=kmalloc(sizeof(struct PTE));
	tmp1->next->PTE_P=0;
	tmp1->next->read=0;
	tmp1->next->write=0;
	tmp1->next->exe=0;
	tmp1->next->next = NULL;
	}
	//HEAP
	as->heap_base=vaddr+sz;
	as->heap_top=as->heap_base;


//	(void)as;
//	(void)vaddr;
//	(void)sz;
//	(void)readable;
//	(void)writeable;
//	(void)executable;
	return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	struct region *tmp=as->region;
	while (tmp->next!=NULL){
		if (tmp->write==0) {tmp->write=1;tmp->flag=1;}
		tmp=tmp->next;
	}
	//(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	struct region *tmp=as->region;
		while (tmp->next!=NULL){
			if (tmp->flag==1) {tmp->write=0;}
			tmp=tmp->next;
		}

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	//(void)as;
	as->stack_base= USERSTACK;
	as->stack_top = USERSTACK;
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
}

