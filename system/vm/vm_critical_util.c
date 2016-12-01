/*
 * 
 * Copyright (c) 2016 Simon Schmidt
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <vm/vm_top.h>
#include <vm/vm_as.h>
#include <vm/vm_seg.h>
#include <vm/vm_mem.h>
#include <vm/vm_range.h>
#include <vm/vm_priv.h>
#include <sysarch/pages.h>
#include <sys/kernslice.h>
#include <sys/physmem_alloc.h>
#include <vm/pmap.h>
#include <kern/zalloc.h>
#include <stdio.h>

#ifdef SYSARCH_PAGESIZE_SHIFT
#define MUL_PAGESIZE(x)  ((x)<<SYSARCH_PAGESIZE_SHIFT)
#define DIV_PAGESIZE(x)  ((x)>>SYSARCH_PAGESIZE_SHIFT)
#define ROUND_DOWN(x) x &= ~((1<<SYSARCH_PAGESIZE_SHIFT)-1)
#else
#define MUL_PAGESIZE(x)  ((x)*SYSARCH_PAGESIZE)
#define DIV_PAGESIZE(x)  ((x)/SYSARCH_PAGESIZE)
#define ROUND_DOWN(x) x -= (x%SYSARCH_PAGESIZE)
#endif


#define PMBM(slice) ((slice)->ks_memory_allocator)

#define VM_PROT_KMEM  VM_PROT_READ | VM_PROT_WRITE


static struct vm_mem* vm_mem_kcfilled(pmap_t pmap, vaddr_t begin,vaddr_t size,struct kernslice* slice){
	/*
	 * Assumption: size is a multiple of SYSARCH_PAGESIZE.
	 */
	vaddr_t N = DIV_PAGESIZE(size); /* XXX: this should be rounded up by default. */
	vaddr_t i,j,M;
	paddr_t page;
	vm_range_t range,next = 0;
	
	/* This shouldn't happen. */
	if(N<1) return 0;
	
	/*
	 * Allocate the memory structure.
	 */
	vm_mem_t mem = vm_mem_alloc_critical();
	if(!mem)return 0;
	
	/*
	 * If the mapping is only one memory page big, put-in the page directly.
	 */
	if(N==1){
		if(!vm_phys_alloc(PMBM(slice),&page)) goto FAILED;
		mem->mem_phys_type = VMM_IS_PGADDR;
		mem->mem_pgaddr = page;
		pmap_enter(pmap,begin,page,VM_PROT_KMEM,0);
		return mem;
	}
	
	/*
	 * Otherwise, allocate one or more vm_range_t structs.
	 */
	range = vm_range_alloc_critical(slice);
	if(!range) goto FAILED;
	mem->mem_pmrange = range;
	mem->mem_phys_type = VMM_IS_PMRANGE;
	
	/*
	 * Allocate Physical Memory and put it into the vm_range_t
	 */
	M = N;
	if(M>VM_RANGE_NUM) M = VM_RANGE_NUM;
	for(j=0;j<M;++j){
		if(!vm_phys_alloc(PMBM(slice),&page)) goto FAILED2;
		range->rang_pages[j].page_addr = page;
		vm_range_bmset(range,j);
		pmap_enter(pmap,begin,page,VM_PROT_KMEM,0);
		begin += SYSARCH_PAGESIZE;
	}
	
	/*
	 * Allocate subsequent ranges.
	 */
	for(i=VM_RANGE_NUM;i<N;i+=VM_RANGE_NUM){
		/*
		 * Allocate another range and append it to the list.
		 */
		next = vm_range_alloc_critical(slice);
		if(!next) goto FAILED;
		range->rang_next = next;
		range = next;
		
		/*
		 * XXX: This is duplicate code. See above.
		 */
		M = N-i;
		if(M>VM_RANGE_NUM) M = VM_RANGE_NUM;
		for(j=0;j<M;++j){
			if(!vm_phys_alloc(PMBM(slice),&page)) goto FAILED2;
			range->rang_pages[j].page_addr = page;
			vm_range_bmset(range,j);
			pmap_enter(pmap,begin,page,VM_PROT_KMEM,0);
			begin += SYSARCH_PAGESIZE;
		}
	}
	return mem;
	
	/*
	 * On error, we need to free allocated resources.
	 */
	FAILED2:
	range = mem->mem_pmrange;
	while(range){
		for(j=0;j<VM_RANGE_NUM;++j){
			if(vm_range_bmlkup(range,j))
				vm_phys_free(PMBM(slice),range->rang_pages[j].page_addr);
		}
		next = range->rang_next;
		zfree(range);
		range = next;
	}
	FAILED:
	zfree(mem);
	return 0;
}

static int vm_seg_kcfill(vm_seg_t seg,pmap_t pmap){
	vaddr_t size = (seg->seg_end - seg->seg_begin)+1;
	seg->seg_prot = VM_PROT_KMEM;
	seg->seg_mem = vm_mem_kcfilled(pmap,seg->seg_begin,size,pmap_kernslice(pmap));
	if(!(seg->seg_mem)){
		pmap_remove(pmap,seg->seg_begin,seg->seg_end);
		return 0;
	}
	return 1;
}

int vm_alloc_critical(vaddr_t *addr /* [out] */,vaddr_t *size /* [in/out]*/){
	int res = 0;
	vm_as_t as;
	vm_seg_t seg;
	printf("vm_alloc_critical(%d)\n",(int)(*size));
	
	/*
	 * Round-Up the size.
	 */
	vaddr_t lsiz = *size + SYSARCH_PAGESIZE - 1;
	ROUND_DOWN(lsiz);
	
	as = vm_as_get_kernel();
	
	kernlock_lock(&(as->as_lock));
	
	seg = vm_create_entry_critical(as,lsiz);
	if(!seg) goto endKalloc;
	
	if(!vm_seg_kcfill(seg,as->as_pmap)) goto endKalloc; /* TODO: Remove segment. */
	
	*addr = seg->seg_begin;
	*size = (seg->seg_end - seg->seg_begin)+1;
	res = 1;
	
endKalloc:
	kernlock_unlock(&(as->as_lock));
	return res;
}

