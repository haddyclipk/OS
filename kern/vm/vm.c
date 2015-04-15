/*
 * vm.c
 *
 *  Created on: Apr 14, 2015
 *      Author: trinity
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <thread.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include<machine/vm.h>

struct lock* coremap_lk;
static int bootstrap=0;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static int pnum;
//#define DUMBVM_STACKPAGES    12

void vm_bootstrap(void){
	paddr_t first,last,freeadrs;
	paddr_t x=0;
	coremap_lk=lock_create("coremap_lock");
	ram_getsize(&first,&last);
	pnum=last/PAGE_SIZE;
	coremap=(struct coremap_entry*)PADDR_TO_KVADDR(first);
	freeadrs= first+pnum*sizeof(struct coremap_entry);
	for (int i=0; i<pnum;i++){
		coremap[i].va=PADDR_TO_KVADDR(x+i*PAGE_SIZE);
		if(x+i*PAGE_SIZE<freeadrs) coremap[i].pgstate=FIXED;
		else {coremap[i].pgstate=FREE;
			coremap[i].chunk=1;}
		}

	bootstrap=1;
}
static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);

	spinlock_release(&stealmem_lock);
	return addr;
}



/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages){
	if (bootstrap==0){return PADDR_TO_KVADDR(getppages(npages));}
	else return page_nalloc(npages);
}
vaddr_t page_nalloc(int npages){
	lock_acquire(coremap_lk);
	int i;
	for (i=0;i<pnum;i++){
		if (coremap[i].pgstate==FREE){
			for (int k=1;k<npages;k++){
				if(coremap[i+k].pgstate==FREE)coremap[i].chunk++;
				else break;
			}
		if (coremap[i].chunk==npages) {
			break;
						}

		}
	}
	if(coremap[i].chunk!=npages){lock_release(coremap_lk);return EFAULT;}
	else {
		for (int j=0;j<npages;j++){
		make_page_avail((struct coremap_entry*)&coremap[i+j]);
		coremap[i+j].pgstate=DIRTY;}
	}
	lock_release(coremap_lk);
	return coremap[i].va;


}
vaddr_t page_alloc(void){
	lock_acquire(coremap_lk);
	int i;
	for(i=0;i<pnum;i++){
		if(coremap[i].pgstate==FREE)break;
	}
	if(coremap[i].pgstate!=FREE){
		return EFAULT;
		}
	make_page_avail(&coremap[i]);
	lock_release(coremap_lk);
	return coremap[i].va;

}
 void make_page_avail(struct coremap_entry* coremap){
	coremap->pgstate=DIRTY;
	bzero((void *)coremap->va,PAGE_SIZE);
}
void free_kpages(vaddr_t addr){
	int i;
	for (i=0;i<pnum;i++){
		if(coremap[i].va==addr)break;
	}
	if(coremap[i].va!=addr) return;
	if(coremap[i].pgstate==FIXED)return;
	int chunk=coremap[i].chunk;
	for (int j=0;j<chunk;j++){
		coremap[i+j].pgstate=FREE;
		coremap[i+j].chunk=1;
		}
}



/* TLB shootdown handling called from interprocessor_interrupt */
void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

/* Fault handling function called by trap code */
int
vm_fault(int faulttype, vaddr_t faultaddress)
{	(void) faultaddress;
	(void)faulttype;
	return 0;
}

//struct addrspace *
//as_create(void)
//{
//	struct addrspace *as = kmalloc(sizeof(struct addrspace));
//	if (as==NULL) {
//		return NULL;
//	}
//
//	as->as_vbase1 = 0;
//	as->as_pbase1 = 0;
//	as->as_npages1 = 0;
//	as->as_vbase2 = 0;
//	as->as_pbase2 = 0;
//	as->as_npages2 = 0;
//	as->as_stackpbase = 0;
//
//	return as;
//}
//
//void
//as_destroy(struct addrspace *as)
//{
//	kfree(as);
//}
//
//void
//as_activate(struct addrspace *as)
//{
//	int i, spl;
//
//	(void)as;
//
//	/* Disable interrupts on this CPU while frobbing the TLB. */
//	spl = splhigh();
//
//	for (i=0; i<NUM_TLB; i++) {
//		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
//	}
//
//	splx(spl);
//}
//
//int
//as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
//		 int readable, int writeable, int executable)
//{
//	size_t npages;
//
//	/* Align the region. First, the base... */
//	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
//	vaddr &= PAGE_FRAME;
//
//	/* ...and now the length. */
//	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;
//
//	npages = sz / PAGE_SIZE;
//
//	/* We don't use these - all pages are read-write */
//	(void)readable;
//	(void)writeable;
//	(void)executable;
//
//	if (as->as_vbase1 == 0) {
//		as->as_vbase1 = vaddr;
//		as->as_npages1 = npages;
//		return 0;
//	}
//
//	if (as->as_vbase2 == 0) {
//		as->as_vbase2 = vaddr;
//		as->as_npages2 = npages;
//		return 0;
//	}
//
//	/*
//	 * Support for more than two regions is not available.
//	 */
//	kprintf("dumbvm: Warning: too many regions\n");
//	return EUNIMP;
//}
//
//static
//void
//as_zero_region(paddr_t paddr, unsigned npages)
//{
//	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
//}
//
//int
//as_prepare_load(struct addrspace *as)
//{
//	KASSERT(as->as_pbase1 == 0);
//	KASSERT(as->as_pbase2 == 0);
//	KASSERT(as->as_stackpbase == 0);
//
//	as->as_pbase1 = getppages(as->as_npages1);
//	if (as->as_pbase1 == 0) {
//		return ENOMEM;
//	}
//
//	as->as_pbase2 = getppages(as->as_npages2);
//	if (as->as_pbase2 == 0) {
//		return ENOMEM;
//	}
//
//	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
//	if (as->as_stackpbase == 0) {
//		return ENOMEM;
//	}
//
//	as_zero_region(as->as_pbase1, as->as_npages1);
//	as_zero_region(as->as_pbase2, as->as_npages2);
//	as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);
//
//	return 0;
//}
//
//int
//as_complete_load(struct addrspace *as)
//{
//	(void)as;
//	return 0;
//}
//
//int
//as_define_stack(struct addrspace *as, vaddr_t *stackptr)
//{
//	KASSERT(as->as_stackpbase != 0);
//
//	*stackptr = USERSTACK;
//	return 0;
//}
//
//int
//as_copy(struct addrspace *old, struct addrspace **ret)
//{
//	struct addrspace *new;
//
//	new = as_create();
//	if (new==NULL) {
//		return ENOMEM;
//	}
//
//	new->as_vbase1 = old->as_vbase1;
//	new->as_npages1 = old->as_npages1;
//	new->as_vbase2 = old->as_vbase2;
//	new->as_npages2 = old->as_npages2;
//
//	/* (Mis)use as_prepare_load to allocate some physical memory. */
//	if (as_prepare_load(new)) {
//		as_destroy(new);
//		return ENOMEM;
//	}
//
//	KASSERT(new->as_pbase1 != 0);
//	KASSERT(new->as_pbase2 != 0);
//	KASSERT(new->as_stackpbase != 0);
//
//	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
//		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
//		old->as_npages1*PAGE_SIZE);
//
//	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
//		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
//		old->as_npages2*PAGE_SIZE);
//
//	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
//		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
//		DUMBVM_STACKPAGES*PAGE_SIZE);
//
//	*ret = new;
//	return 0;
//}
//