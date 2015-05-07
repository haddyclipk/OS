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

struct spinlock coremap_lk=SPINLOCK_INITIALIZER;
struct spinlock tlb_lk=SPINLOCK_INITIALIZER;

static int bootstrap=0;
static int free_start=0;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static int pnum;
static void pp()
{
	return ;
}
//#define DUMBVM_STACKPAGES    12

void vm_bootstrap(void){
	paddr_t first,last,freeadrs;
	//paddr_t x=0;
	//coremap_lk=lock_create("coremap_lock");
	//KASSERT(coremap_lk!=NULL);
	ram_getsize(&first,&last);
	pnum=last/PAGE_SIZE;
	coremap=(struct coremap_entry*)PADDR_TO_KVADDR(first);
	freeadrs= first+pnum*sizeof(struct coremap_entry);
	free_start=freeadrs/PAGE_SIZE;
	for (int i=0; i<pnum;i++){
		coremap[i].va=0;
		coremap[i].pa=PADDR_TO_KVADDR(i*PAGE_SIZE);
		if(i*PAGE_SIZE<(int)freeadrs) coremap[i].pgstate=FIXED;
		else {coremap[i].pgstate=FREE;
			coremap[i].chunk=1;
			coremap[i].as=NULL;}
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
	//int i;
	spinlock_acquire(&coremap_lk);
	int i;
	int p=0;
	//paddr_t t=0;
	//vaddr_t v;
	for (i=free_start;i<pnum;i++){
		if (coremap[i].pgstate==FREE){
			for (int k=0;k<npages;k++){
				if(coremap[i+k].pgstate==FREE)p++;
				else break;
			}
		if (p==npages) {
			coremap[i].chunk=p; break;
						}
		else{p=0;}

		}
	}
	if(coremap[i].chunk!=npages){spinlock_release(&coremap_lk);return ENOMEM;}
	else {
		for (int j=0;j<npages;j++){
			//v=PADDR_TO_KVADDR(t+(i+j)*PAGE_SIZE);
		make_page_avail(&coremap[i+j]);
		coremap[i+j].va=coremap[i+j].pa;
		coremap[i+j].pid=curthread->t_pid;
		coremap[i+j].pgstate=DIRTY;
		coremap[i+j].as=NULL;
		}
	}
	spinlock_release(&coremap_lk);
	return PADDR_TO_KVADDR((i*PAGE_SIZE));


}
vaddr_t page_alloc(vaddr_t va){

	//paddr_t t=0;
	//vaddr_t v;
	int i;
	spinlock_acquire(&coremap_lk);
	for(i=free_start;i<pnum;i++){
		if(coremap[i].pgstate==FREE)break;
	}
	if(coremap[i].pgstate!=FREE){
		pp();
		return EFAULT;
		}
	//v=PADDR_TO_KVADDR((t+i*PAGE_SIZE));
	make_page_avail(&coremap[i]);
	coremap[i].pid=curthread->t_pid;
	coremap[i].va=va;
	coremap[i].pgstate=DIRTY;
	coremap[i].as=curthread->t_addrspace;//update addresspace
	spinlock_release(&coremap_lk);
	return PADDR_TO_KVADDR(i*PAGE_SIZE);

}
 void make_page_avail(struct coremap_entry *coremap){
	 //KASSERT(coremap->pgstate==FREE&&coremap->chunk==1);
	coremap->pgstate=DIRTY;
	bzero((void *)coremap->pa,PAGE_SIZE);
}
// static void shottlb(vaddr_t addr){
//	 spinlock_acquire(&tlb_lk);
//	 paddr_t pa=0;
//	 int m=tlb_probe(addr,pa);
//	 if (m>=0)tlb_write(TLBHI_INVALID(m),TLBLO_INVALID(),m);
// }
void free_kpages(vaddr_t addr){
	addr&=PAGE_FRAME;
//	struct PTE *pt=curthread->t_addrspace->ptable;
//	int count=curthread->t_addrspace->pagenum;
//	int i;
//	int k=0;
//	for (i=0;i<count;i++){if(pt->va==addr) {k=pt->pa/PAGE_SIZE;}
//	pt=pt->next;}
//	KASSERT(k!=0);
	int i;
	spinlock_acquire(&coremap_lk);
	for (i=free_start;i<pnum;i++){
		if(coremap[i].va==addr&&coremap[i].pid==curthread->t_pid)break;
	}
	if(coremap[i].va!=addr) {spinlock_release(&coremap_lk);return;}
	//coremap[i].as->ptable
	if(coremap[i].pgstate==FIXED){spinlock_release(&coremap_lk);return;}

	int chunk=coremap[i].chunk;

	for (int j=0;j<chunk;j++){
		//page_free(addr+i*PAGE_SIZE);
		coremap[i+j].pid=0;
		coremap[i+j].pgstate=FREE;
		coremap[i+j].chunk=1;

		coremap[i+j].va=0;///////////////
//		//unmap
		coremap[i+j].as=NULL;
		}
	spinlock_release(&coremap_lk);
}

void page_free(vaddr_t addr){
	addr&=PAGE_FRAME;
//		struct PTE *pt=curthread->t_addrspace->ptable;
//		int count=curthread->t_addrspace->pagenum;
		int i;
//		int k=0;
//		for (i=0;i<count;i++){if(pt->va==addr) {k=pt->pa/PAGE_SIZE;shottlb(addr);pt->va=0;pt->pa=0;}
//		pt=pt->next;}
//		KASSERT(k!=0);
		spinlock_acquire(&coremap_lk);
	for (i=free_start;i<pnum;i++){
		if(coremap[i].va==addr&&coremap[i].pid==curthread->t_pid)break;
	}
	//KASSERT(i!=pnum);
	if(coremap[i].va!=addr) {spinlock_release(&coremap_lk);return;}
	//KASSERT(coremap[i].pgstate!=FIXED);
	if(coremap[i].pgstate==FIXED){spinlock_release(&coremap_lk);return;}




		coremap[i].va=0;
		coremap[i].pgstate=FREE;
		coremap[i].chunk=1;
		coremap[i].as=NULL;
		coremap[i].pid=0;

	spinlock_release(&coremap_lk);
}




/* TLB shootdown handling called from interprocessor_interrupt */
void
vm_tlbshootdown_all(void)
{
	struct spinlock s_lock;
	spinlock_init(&s_lock);
	spinlock_acquire(&s_lock);
	for(uint32_t i = 0; i<NUM_TLB; i++){
		tlb_write(TLBHI_INVALID(i),TLBLO_INVALID(), i);	}
	spinlock_release(&s_lock);
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}
static void bp()
{
	return ;
}
static void cp()
{
	return ;
}

/* Fault handling function called by trap code */
int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	//check vaddr_t is valid
		struct PTE *tmp;
		struct PTE *p2=NULL;
		int flag=0;

		int tlb_index;
		//paddr_t pad = 0;
		if(curthread->t_addrspace == NULL) return EFAULT;
		struct addrspace *as=curthread->t_addrspace;


		KASSERT(faultaddress < MIPS_KSEG0);
		KASSERT(as->ptable != NULL);
		KASSERT(as->region != NULL);
		KASSERT(as->heap_base != 0);
		KASSERT(as->heap_top != 0);
	KASSERT((as->ptable->va & PAGE_FRAME) == as->ptable->va);
		//cp();
		//KASSERT((as->ptable->pa & PAGE_FRAME) == as->ptable->pa);

				struct region *reg=as->region;
		vaddr_t vadr = faultaddress;
		faultaddress &= PAGE_FRAME;

		struct spinlock s_lock;
		struct PTE *p = as->ptable;
		paddr_t padr = 0;
		//vaddr_t vadr = faultaddress;
		spinlock_init(&s_lock);
		while (reg!=NULL){
					bp();
					if ((faultaddress>=reg->vbase) && (faultaddress<(reg->vbase+PAGE_SIZE*reg->psize))) flag=1;
					reg=reg->next;
					bp();
				}
				bp();
				if ((vadr>=as->heap_base) && vadr<(as->heap_top)) flag=1;
				if (vadr>=(as->stack_base) && vadr<(as->stack_top)) flag=1;
				if (flag==0) {cp();panic("flag=0");}

		// check what fault it is
		if (faulttype!=0 && faulttype!=1 && faulttype!=2){panic("unknown fault");}
		//vm_read
		if (faulttype==0||faulttype==1){
			spinlock_acquire(&s_lock);
			int m=tlb_probe(faultaddress, padr);
			if (m>=0&&m<=NUM_TLB){spinlock_release(&s_lock);panic("TLB wrong");}
			while(p != NULL){
				if(p->next==NULL){tmp=p;}
				if(p->va == faultaddress ){
					if( p->PTE_P == 1){
						//if(p->write==1)
					padr = p->pa | TLBLO_DIRTY | TLBLO_VALID;
						//else padr = p->pa | TLBLO_VALID;
					}
				}
//					else{p->pa=KVADDR_TO_PADDR(page_alloc());
//					int j=(p->pa)/PAGE_SIZE;
//					p->PTE_P=1;
//					//p->next=NULL;
//					//coremap update
//					spinlock_acquire(&coremap_lk);
//					coremap[j].va=p->va;
//					coremap[j].as=as;
//					coremap[j].pgstate=DIRTY;
//					spinlock_release(&coremap_lk);
//					//if(p->write==1)
//					padr = p->pa | TLBLO_DIRTY | TLBLO_VALID;
//					//KASSERT((padr & PAGE_FRAME) == padr);//else padr = p->pa | TLBLO_VALID;
//					}
//				break;}

				p = p->next;
			}
			if(padr==0) {tmp->next=kmalloc(sizeof(struct PTE));
			p=tmp->next;
			p->exe=1;
			p->read=1;
			p->write=1;
			p->va=faultaddress;
			p->PTE_P=1;

			p->pa=KVADDR_TO_PADDR(page_alloc(faultaddress));
			as->pagenum++;

			//int j=(p->pa)/PAGE_SIZE;



			p->next=NULL;
			padr = p->pa | TLBLO_DIRTY | TLBLO_VALID;
			}
			bp();
			//KASSERT((padr & PAGE_FRAME) == padr);
			tlb_random(faultaddress, padr);
			spinlock_release(&s_lock);
		}
		//vm_write
//		if(faulttype==1){
//		spinlock_acquire(&s_lock);
//			int m=tlb_probe(faultaddress, padr);
//			if (m>=0&&m<=NUM_TLB){spinlock_release(&s_lock);return 0;}
//
//			while(p!= NULL){
//				if(p->va == faultaddress){
//					bp();
//					p2=p;
//					break;}
//					p=p->next;
//					}
//				bp();
//
//				if( p2->PTE_P == 1){
//
//				padr = p2->pa | TLBLO_DIRTY | TLBLO_VALID;
//							}
//								else{
//									padr=page_alloc();
//									padr = padr | TLBLO_DIRTY | TLBLO_VALID;
//								}
//
//
//bp();
//						tlb_random(faultaddress, padr);
//						spinlock_release(&s_lock);
//		}
		//vm_readonly
		if(faulttype==2){
			//p = curthread->t_addrspace->ptable;
			while(p != NULL){
				if(p->va == faultaddress){
					p2 = p;
					break;
				}
				p = p->next;
			}
			if(p2==NULL) panic("read_only err");
			if(p2->write!=1 )panic("Not writable");
			spinlock_acquire(&s_lock);
			bp();
			tlb_index=tlb_probe(faultaddress,padr);
			if(tlb_index<0) panic("tlb_readonly fault");
//			for(int i = 0; i<NUM_TLB; i++){
//				tlb_read(&vadr,&padr, i);
			tlb_read(&vadr,&padr,tlb_index);


				padr = padr| TLBLO_DIRTY |TLBLO_VALID;
				KASSERT((padr & PAGE_FRAME) == padr);
				tlb_write(vadr, padr, tlb_index);
				spinlock_release(&s_lock);
				//coremap
		}
bp();

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
