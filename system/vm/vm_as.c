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
#include <vm/vm_as.h>
#include <vm/vm_seg.h>
#include <vm/vm_priv.h>
#include <vm/pmap.h>
#include <kern/zalloc.h>

static zone_t vm_as_zone; /* Zone for vm_as structures. */

static struct vm_as kernel_as;

void vm_as_init(){
	vm_as_zone = zinit(sizeof(struct vm_as),ZONE_AUTO_REFILL,"VM address space");
	kernel_as.as_segs = 0;
	kernel_as.as_pmap = pmap_kernel();
	kernlock_init(&(kernel_as.as_lock));
	pmap_get_address_range(kernel_as.as_pmap, &(kernel_as.as_begin),&(kernel_as.as_end));
}

void vm_as_refill(){
	zrefill(vm_as_zone,64,64);
}

static int vm_find_free(vm_as_t as, vm_seg_t dseg, vaddr_t lrp /* Last relative pointer (size -1) */){
	vm_bintree_t * __restrict__ bt;
	vm_seg_t seg;
	vaddr_t begin = as->as_begin;
	vaddr_t end = begin+lrp;
	
	while(end<=as->as_end){
		bt = bt_ceiling(&(as->as_segs),begin);
		if(bt && *bt){
			seg = (vm_seg_t)((*bt)->V);
			if(end>=seg->seg_begin){
				begin = seg->seg_end+1;
				end = begin+lrp;
				continue;
			}
		}
		dseg->seg_begin = begin;
		dseg->seg_end   = end;
		return 1;
	}
	return 0;
}

vm_as_t vm_as_get_kernel(){ return &kernel_as; }

struct vm_seg *vm_create_entry(vm_as_t as, vaddr_t size) {
	int kernel = as==&kernel_as;
	vm_bintree_t entry;
	
	vm_seg_t seg = vm_seg_alloc(kernel);
	if(!seg) return 0;
	
	if(!vm_find_free(as,seg,size-1)) {
		zfree(seg);
		return 0;
	}
	
	vm_seg_initobj(seg);
	entry = &(seg->_bt_node);
	bt_insert(&(as->as_segs),&entry);
	if(entry) { /* Insert failed. */
		zfree(seg);
		return 0;
	}
	return seg;
}

struct vm_seg *vm_create_entry_critical(vm_as_t as, vaddr_t size) {
	vm_bintree_t entry;
	
	vm_seg_t seg = vm_seg_alloc_critical();
	if(!seg) return 0;
	
	if(!vm_find_free(as,seg,size-1)) {
		zfree(seg);
		return 0;
	}
	
	vm_seg_initobj(seg);
	entry = &(seg->_bt_node);
	bt_insert(&(as->as_segs),&entry);
	if(entry) { /* Insert failed. */
		zfree(seg);
		return 0;
	}
	return seg;
}

