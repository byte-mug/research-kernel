/*
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
#include <kern/sched.h>
#include <kern/zalloc.h>
#include <libkern/panic.h>
#include <sys/cpu.h>
#include <sys/thread.h>
#include <string.h>

static const signed int sched_prios[SCHED_NRQS] = {
	1, 5, 11, 19, 28, 38, 49, 60, 72, 84,
	97, 110, 123, 137, 151, 165, 180, 195,
	210, 225, 240, 256, 272, 288, 304, 320,
	337, 354, 371, 388, 405, 422
};


typedef struct thread* threadp_t;

static inline linked_ring_t sched_elem(struct thread* thread){
	linked_ring_t ring = &(thread->t_queue_entry);
	ring->data = thread;
	return ring;
}

/*
 * The scheduler-queue consists of 32 priority queues (or SCHED_NRQS). Unlike
 * other priority queues, the queues themself have priorities themself, expressed
 * as decaying values.
 *
 * The scheduler-function essentially performs two steps:
 *
 * The first step is to 'reset' the 'decaying values' of all priorities with their
 * run-queues empty; and to decrease ('decay') the 'decaying values' of all other
 * priority.
 *
 * for(i=0; i<SCHED_NRQS; ++i)
 *     if( ! sched_runnable(scheduler,i) )
 *        scheduler->sched_run_decay[i] = sched_prios[i];
 *     else
 *        scheduler->sched_run_decay[i] --;
 *
 * The second step is to find, among the priorities with non-empty run-queues,
 * the priority with the lowest 'decaying value'.
 *
 * for(i=0; i<SCHED_NRQS; ++i)
 *     if( sched_runnable(scheduler,i) )
 *        find_the_lowest( scheduler->sched_run_decay[i] );
 *
 * Finally, if any value as been found, reset it's 'decaying value':
 *
 *   scheduler->sched_run_decay[i] = sched_prios[i];
 */

#define sched_runnable(scheduler,i) linked_ring_empty(&(scheduler->sched_run_ring[i]))
static struct thread* sched_schedule_next(struct scheduler* scheduler){
	int i;
	int mi = -1;
	signed int mdecay = 0; /* Initialized becaus of warnings. */
	linked_ring_t elem;
	
	/*
	 * The algorithm is implemented as one single loop.
	 * For every priority (i) do:
	 */
	for(i=0; i<SCHED_NRQS; ++i){
		/*
		 * If run-queue of the priority (i) is empty, then reset the
		 * 'decaying value', and skip to the rest of the loop-body!
		 */
		if(!sched_runnable(scheduler,i)){
			scheduler->sched_run_decay[i] = sched_prios[i];
			continue;
		}
		
		/*
		 * If we get here, the priority has a non-empty run-queue.
		 *
		 * We decrease the 'decaying value'.
		 */
		scheduler->sched_run_decay[i] --;
		
		/*
		 * If we didn't find the any priority yet (mi<0), or if the
		 * priority has a lower 'decaying value' than the last one, we
		 * found; Then choose that one over the previous one.
		 */
		if( (mi<0) || (scheduler->sched_run_decay[i] < mdecay)) {
			mdecay = scheduler->sched_run_decay[i];
			mi = i;
		}
	}
	
	/* If we din't find any one, return. */
	if(mi<0) return 0;
	
	/* Reset the 'decaying value' of the found priority. */
	scheduler->sched_run_decay[mi] = sched_prios[mi];
	
	/* Remove an Element from the end of the queue. */
	elem = scheduler->sched_run_ring[mi].prev;
	linked_ring_remove(elem);
	return (struct thread*)(elem->data);
}

static inline struct thread* sched_schedule_idle(struct scheduler* scheduler){
	return scheduler->sched_idle;
}

static void sched_reenqueue(struct scheduler* scheduler, struct thread* thread){
	/* Do not enqueue the idle-process. */
	if(thread == scheduler->sched_idle) return;
	
	int i = (thread->t_priority) % SCHED_NRQS;
	
	/* Empty run-queues get reseted. */
	if(!sched_runnable(scheduler,i)) scheduler->sched_run_decay[i] = sched_prios[i];
	
	/* Insert at the begin of the list. */
	linked_ring_insert( &(scheduler->sched_run_ring[i]), sched_elem(thread), /*after=*/ 1 );
}

static zone_t sched_zone; /* Scheduler allocator. */

void sched_init(){
	/*
	 * XXX
	 * Ideally, the 'struct scheduler' instance is allocated from the
	 * kernslice, the scheduler is running on.
	 */
	sched_zone = zinit(sizeof(struct scheduler),ZONE_AUTO_REFILL,"Scheduler allocator");
}

void sched_instanciate(struct cpu* cpu){
	int i;
	struct scheduler* scheduler;
	
	/* Allocate a new 'struct scheduler' instance for this CPU. */
	scheduler = (struct scheduler*)zalloc(sched_zone);
	if(!scheduler) panic("Can't allocate the scheduler core.");
	
	/* Initialize the instance. */
	memset((void*)scheduler,0,sizeof(struct scheduler));
	kernlock_init(&(scheduler->sched_lock));
	
	/* Initialize sched_run_decay-variables. */
	for(i=0; i<SCHED_NRQS; ++i){
		scheduler->sched_run_decay[i] = sched_prios[i];
		linked_ring_init(&(scheduler->sched_run_ring[i]));
	}
	
	/* Assign the instance. */
	cpu->cpu_scheduler = scheduler;
}

/*
 * Inserts a new thread into the scheduler of a given CPU.
 */
void sched_insert(struct cpu* cpu, struct thread* thread){
	threadp_t myself;
	struct scheduler* scheduler;
	myself = kernel_get_current_thread();
	scheduler = cpu->cpu_scheduler;
	
	/*
	 * Set the thread's current cpu.
	 */
	thread->t_current_cpu = cpu;
	
	/*
	 * Set the THREAD_SF_LOCK_SCHED-flag and lock the scheduler.
	 */
	myself->t_stateflags |= THREAD_SF_LOCK_SCHED;
	kernlock_lock(&(scheduler->sched_lock));
	
	/*
	 * Insert the thread into the run-queue.
	 */
	sched_reenqueue(scheduler,thread);
	
	/*
	 * Increment the thread count.
	 */
	scheduler->sched_thread_count ++;
	
	/*
	 * Unlock the scheduler, and get rid of the THREAD_SF_LOCK_SCHED-flag.
	 */
	kernlock_unlock(&(scheduler->sched_lock));
	myself->t_stateflags &= ~THREAD_SF_LOCK_SCHED;
}

/*
 * Remove a thread out of the scheduler of a given CPU.
 */
struct thread* sched_remove(struct cpu* cpu){
	threadp_t myself,thread;
	struct scheduler* scheduler;
	myself = kernel_get_current_thread();
	scheduler = cpu->cpu_scheduler;
	
	/*
	 * Set the THREAD_SF_LOCK_SCHED-flag and lock the scheduler.
	 */
	myself->t_stateflags |= THREAD_SF_LOCK_SCHED;
	kernlock_lock(&(scheduler->sched_lock));
	
	/*
	 * Removes the next thread.
	 */
	thread = sched_schedule_next(scheduler);
	
	/*
	 * Decrement the thread count.
	 */
	scheduler->sched_thread_count --;
	
	/*
	 * Unlock the scheduler, and get rid of the THREAD_SF_LOCK_SCHED-flag.
	 */
	kernlock_unlock(&(scheduler->sched_lock));
	myself->t_stateflags &= ~THREAD_SF_LOCK_SCHED;
	
	/*
	 * Clear the thread's current cpu.
	 */
	thread->t_current_cpu = 0;
	return thread;
}

/*
 * Performs a Thread-preemption. This function must only be called from within
 * a 'preemption-event'. Usually, this is performed from within an interrupt request
 * through the system timer, however it can also be induced.
 * Interrupts are usually turned off, during such an event (on most platforms).
 */
void sched_preempt(){
	threadp_t othr,nthr;
	struct scheduler* scheduler;
	
	/* Get scheduler. */
	scheduler = kernel_get_current_cpu()->cpu_scheduler;
	
	/* Current thread. */
	othr = kernel_get_current_thread();
	
	if((othr->t_stateflags) & THREAD_SF_LOCK_SCHED)
		/*
		 * Ooops. This thread is currently modifying this (or another)
		 * scheduler. So, don't even touch the scheduler.
		 */
		return;
	
	/* Synchronized{ */
	kernlock_lock(&(scheduler->sched_lock));
	
	/* Get next runnable thread. */
	nthr = sched_schedule_next(scheduler);
	
	/* If there is no next runnable thread, get idle-thread. */
	if(!nthr) nthr = sched_schedule_idle(scheduler);
	
	/* Switch Threads. */
	nthr->t_stateflags &= ~THREAD_SF_PREEMPT;
	kernel_set_current_thread(nthr);
	othr->t_stateflags |= THREAD_SF_PREEMPT;
	
	/* Enqueue the old thread to the runnable queue. */
	sched_reenqueue(scheduler,othr);
	
	kernlock_unlock(&(scheduler->sched_lock));
	/* } */
}

