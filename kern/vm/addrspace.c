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
#include <current.h>
/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */
//extern struct spinlock coremap_lk;
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

	as->ptable=NULL;//kmalloc(sizeof(struct PTE));
	as->pagenum=0;
//	as->ptable->PTE_P=0;
//	as->ptable->read=0;
//	as->ptable->write=0;
//	as->ptable->exe=0;
//	as->ptable->next = NULL;

	as->region=NULL;//kmalloc(sizeof(struct region));
//	as->region->exe=0;
//	as->region->read=0;
//	as->region->write=0;
//	as->region->flag=0;
//	as->region->next = NULL;

	as->heap_base=0;
	as->heap_top=0;
	as->stack_base=USERSTACK-12*PAGE_SIZE;
	as->stack_top=USERSTACK;

	return as;
}
static void ptecpy(struct addrspace* newas,struct PTE *new,struct PTE *old){
	new->PTE_P=old->PTE_P;
	new->va=old->va;
	new->pa=KVADDR_TO_PADDR(page_alloc(newas,old->va));
	memmove((void *)PADDR_TO_KVADDR(new->pa),(const void *)PADDR_TO_KVADDR(old->pa),PAGE_SIZE);
	new->read=old->read;
	new->write=old->write;
	new->exe=old->exe;
}
static void regcpy(struct region *new,struct region *old){
	new->vbase=old->vbase;
	new->flag=old->flag;
	new->psize=old->psize;
	new->read=old->read;
	new->write=old->write;
	new->exe=old->exe;
}
int
as_copy( struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL || old == NULL) {
		return ENOMEM;
	}


	newas->heap_base = old->heap_base;
	newas->heap_top = old->heap_top;
	newas->stack_base = old->stack_base;
	newas->stack_top = old->stack_top;
	newas->pagenum=old->pagenum;

	//newas->region=old->region;
	//newas->ptable=old->ptable;
//	int pgnumber=old->pagenum;
//	newas->ptable=kmalloc(pgnumber*sizeof(struct PTE));
//	memcpy(newas->ptable, old->ptable,pgnumber*sizeof(struct PTE));

		struct PTE * temp1=old->ptable;


	struct PTE *tmp2;
	struct region * temp3 = old->region;
	struct region * temp4;
	if(old->region==NULL) return 0;
	if(old->ptable==NULL) return 0;
	newas->ptable=kmalloc(sizeof(struct PTE));
	tmp2=newas->ptable;
	ptecpy(newas,tmp2,old->ptable);
	old->ptable=old->ptable->next;
	while(old->ptable!= NULL){
		tmp2->next=kmalloc(sizeof(struct PTE));
		tmp2=tmp2->next;
		ptecpy(newas,tmp2,old->ptable);


		old->ptable = old->ptable->next;
	}
	tmp2->next=NULL;
	old->ptable = temp1;

	newas->region=kmalloc(sizeof(struct region));
	temp4=newas->region;
	regcpy(temp4, old->region);
	old->region=old->region->next;
	while(old->region != NULL){
		temp4->next= kmalloc(sizeof(struct region));
		temp4=temp4->next;
		regcpy(temp4,old->region);

		old->region = old->region->next;
	}
	temp4->next=NULL;
	old->region = temp3;

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

	struct PTE * pt = NULL;
	//struct PTE * temp1 = NULL;
	struct region * reg;
	//struct region * temp2 = NULL;

	while(as->ptable != NULL){  ////clean pagetable
		pt = as->ptable;
		page_free(as->ptable->va,as);
		as->ptable->va=0;
		as->ptable = as->ptable->next;
		//bzero(pt, sizeof(struct PTE));
		kfree(pt);
	}

	while (as->region != NULL)
			{
				reg = as->region;
				//bzero(reg,sizeof(struct region));
				as->region = as->region->next;
				kfree(reg);
			}
	as->pagenum=0;
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
//static void addpte(struct PTE *tmp1, int i, vaddr_t vaddr,  struct addrspace *as,int readable, int writeable, int executable){
//	tmp1->va=vaddr+i*PAGE_SIZE;
////update permission
//	if(readable==4)tmp1->read=1;
//	if(writeable==2)tmp1->write=1;
//	if(executable==1)tmp1->exe=1;
////spinlock_acquire(&coremap_lk);
//
//tmp1->pa=-1;//KVADDR_TO_PADDR(page_alloc(curthread->t_addrspace,tmp1->va));
////int j=(tmp1->pa)/PAGE_SIZE;
//tmp1->PTE_P=1;
//tmp1->next=NULL;
//as->pagenum++;
//coremap update
//coremap[j].pid=curthread->t_pid;
//coremap[j].va=tmp1->va;
//coremap[j].as=as;
//coremap[j].pgstate=DIRTY;
//spinlock_release(&coremap_lk);


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

	int count;
	struct region *tmp=NULL;
	struct region *reg = kmalloc(sizeof(struct region));
	KASSERT(reg!=NULL);

	reg->vbase = vaddr;
	reg->psize = sz / PAGE_SIZE;
	reg->flag=0;
	reg->next=NULL;
	if(readable==4)reg->read=1;
	if(writeable==2)reg->write=1;
	if(executable==1)reg->exe=1;
	//check if 1st one
	if(as->region==NULL){as->region=reg;count=as->region->psize;}
	else{tmp=as->region;
	while(tmp->next != NULL){
			tmp=tmp->next;
		}
		tmp->next=reg;
		count=reg->psize;
	}
	as->heap_base=vaddr+(sz/PAGE_SIZE)*PAGE_SIZE;
		as->heap_top=as->heap_base;





	// update pagetable
//	struct PTE *PT= as->ptable;
//	struct PTE *pt1=kmalloc(sizeof(struct PTE) );
//	KASSERT(pt1!=NULL);
//	addpte(pt1,0,vaddr,as,readable,writeable, executable);
//	struct PTE *tmp1=pt1;
////ap();
//	for(int i=1;i<count;i++){
//		ap();
//
//		tmp1->next=kmalloc(sizeof(struct PTE));
//		tmp1=tmp1->next;
//		KASSERT(tmp1!=NULL);
////
//		addpte(tmp1,i,vaddr,as,readable,writeable, executable);
////		ap();
//	//coremap update
//
//	}
//
//	if (as->ptable==NULL){as->ptable=pt1;}
//	else{while(PT->next!=NULL){
//			PT=PT->next;
//			ap();
//		}
//	PT->next=pt1;
//	ap();
//	}
	//HEAP



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
	while (tmp!=NULL){
		if (tmp->write==0) {tmp->write=1;tmp->flag=1;}
//		struct PTE *PT= as->ptable;
//			struct PTE *pt1=kmalloc(sizeof(struct PTE) );
//			KASSERT(pt1!=NULL);
//			addpte(pt1,0,tmp->vbase,as,tmp->read,tmp->write, tmp->exe);
//			struct PTE *tmp1=pt1;
//		//ap();
//			for(int i=1;i<count;i++){
//				ap();
//
//				tmp1->next=kmalloc(sizeof(struct PTE));
//				tmp1=tmp1->next;
//				KASSERT(tmp1!=NULL);
//		//
//				addpte(tmp1,i,tmp->vbase,as,tmp->read,tmp->write, tmp->exe);
//		//		ap();
//			//coremap update
//
//			}
//
//			if (as->ptable==NULL){as->ptable=pt1;}
//			else{while(PT->next!=NULL){
//					PT=PT->next;
//					ap();
//				}
//			PT->next=pt1;
//			ap();
//			}

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
		while (tmp!=NULL){
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
	//as->stack_base= USERSTACK;
	//as->stack_top = USERSTACK;
	/* Initial user-level stack pointer */
	*stackptr = as->stack_top;
	
	return 0;
}

