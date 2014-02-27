/**
 * thread functions
 *
 * Copyright (C) 2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @ingroup kernel
 * @{
 * @file
 * @author Kaspar Schleiser <kaspar@schleiser.de>
 * @}
 */

#include <errno.h>
#include <stdio.h>

#include "thread.h"
#include "kernel.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"
#include "kernel_internal.h"
#include "bitarithm.h"
#include "hwtimer.h"
#include "sched.h"


inline int thread_getpid()
{
    return active_thread->pid;
}

int thread_getlastpid()
{
    extern int last_pid;
    return last_pid;
}

unsigned int thread_getstatus(int pid)
{
    if (sched_threads[pid] == NULL) {
        return STATUS_NOT_FOUND;
    }

    return sched_threads[pid]->status;
}

const char *thread_getname(int pid)
{
    if (sched_threads[pid] == NULL) {
        return NULL;
    }

    return sched_threads[pid]->name;
}

__INLINE void svc_thread_sleep(void){
	asm volatile("svc #0x7");		/*	call svc	*/
}

void thread_sleep()
{
    dINT();
    sched_set_status((tcb_t *)active_thread, STATUS_SLEEPING);
    eINT();
    thread_yield();
}

int thread_wakeup(int pid)
{
    DEBUG("thread_wakeup: Trying to wakeup PID %i...\n", pid);


    int result = sched_threads[pid]->status;

    if (result == STATUS_SLEEPING) {
        DEBUG("thread_wakeup: Thread is sleeping.\n");
        sched_set_status((tcb_t *)sched_threads[pid], STATUS_RUNNING);


        sched_context_switch_request = 1;

        return 1;
    }
    else {
        DEBUG("thread_wakeup: Thread is not sleeping!\n");


        return STATUS_NOT_FOUND;
    }
}

int thread_measure_stack_usage(char *stack)
{
    unsigned int *stackp = (unsigned int *)stack;

    /* assumption that the comparison fails before or after end of stack */
    while (*stackp == (unsigned int)stackp) {
        stackp++;
    }

    int space = (unsigned int)stackp - (unsigned int)stack;
    return space;
}

__INLINE int svc_thread_create(thread_description *thread){
	int ret = 0;
	asm volatile("mov r0, %[thread_description]": : [thread_description] "r" (thread));
	asm volatile("svc #0x6");		/*	call svc	*/
	asm volatile("mov %[ret], r0": [ret] "=r" (ret));

	return ret;



}

__INLINE int thread_create_desc(thread_description *thread){
	return thread_create(thread->stacksize, thread->priority, thread->flags, thread->function, thread->name);
}


int thread_create(int stacksize, char priority, int flags, void (*function)(void), const char *name)
{
	tcb_t *tcb = get_tcb();
    memory_block_Type *thread_stack = create_mem_block(stacksize);
    tcb->memory = thread_stack;

    tcb->mem_blocks = init_mem_prop(tcb->mem_block_props,thread_stack);



    if (priority >= SCHED_PRIO_LEVELS) {
        return -EINVAL;
    }

    if (flags & CREATE_STACKTEST) {
        /* assign each int of the stack the value of it's address */
        uint32_t *stackmin = (uint32_t *)tcb->memory->start_address;
        uint32_t *stackp = (uint32_t *)tcb->memory->end_address;

        while (stackp > stackmin) {
            *stackp = (uint32_t) stackp;
            stackp--;
        }
    }
    else {
        /* create stack guard */
        *tcb->memory->end_address = (unsigned int)tcb->memory->end_address;
    }

    if (!inISR()) {
        dINT();
    }

    int pid = 0;

    while (pid < MAXTHREADS) {
        if (sched_threads[pid] == NULL) {
            sched_threads[pid] = tcb;
            tcb->pid = pid;
            break;
        }

        pid++;
    }

    if (pid == MAXTHREADS) {
        DEBUG("thread_create(): too many threads!\n");

        if (!inISR()) {
            eINT();
        }

        return -EOVERFLOW;
    }

    tcb->sp = thread_stack_init(function, tcb->memory->end_address, stacksize);
    tcb->stack_size = stacksize;

    tcb->priority = priority;
    tcb->status = 0;

    tcb->rq_entry.data = (unsigned int) tcb;
    tcb->rq_entry.next = NULL;
    tcb->rq_entry.prev = NULL;

    tcb->name = name;

    tcb->wait_data = NULL;

    tcb->msg_waiters.data = 0;
    tcb->msg_waiters.priority = 0;
    tcb->msg_waiters.next = NULL;

    cib_init(&(tcb->msg_queue), 0);
    tcb->msg_array = NULL;

    num_tasks++;

    DEBUG("Created thread %s. PID: %u. Priority: %u.\n", name, cb->pid, priority);

    if (flags & CREATE_SLEEPING) {
        sched_set_status(tcb, STATUS_SLEEPING);
    }
    else {
        sched_set_status(tcb, STATUS_PENDING);

        if (!(flags & CREATE_WOUT_YIELD)) {
            if (!inISR()) {
                eINT();
                thread_yield();
            }
            else {
                sched_context_switch_request = 1;
            }
        }
    }

    if (!inISR() && active_thread != NULL) {
        eINT();
    }

    return pid;
}


