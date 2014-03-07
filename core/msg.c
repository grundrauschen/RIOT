/**
 * kernel messaging implementation
 *
 * Copyright (C) 2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @ingroup kernel_msg
 * @{
 * @file
 * @author Freie Universität Berlin, Computer Systems & Telematics, FeuerWhere project
 * @author Kaspar Schleiser <kaspar@schleiser.de>
 * @author Oliver Hahm <oliver.hahm@inria.fr>
 * @}
 */

#include "kernel.h"
#include "sched.h"
#include "msg.h"
#include "queue.h"
#include "tcb.h"
#include <stddef.h>
#include <irq.h>
#include <cib.h>
#include <inttypes.h>
#include <cpu.h>

#include "flags.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"
#include "thread.h"

static int _msg_receive(msg_t *m, int block);


static int queue_msg(tcb_t *target, msg_t *m)
{
    int n = cib_put(&(target->msg_queue));

    if (n != -1) {
        target->msg_array[n] = *m;
        return 1;
    }

    return 0;
}

void init_queue(void){
	int i;
	for (i=0; i < QUEUE_SIZE ; i++){
		queue[i].is_free = 1;
	}
}

queue_node_t* get_node(void){
	int i;
	for (i=0; i < QUEUE_SIZE ; i++){
		if (queue[i].is_free == 1){
			queue[i].is_free = 0;
			return &queue[i];
		}

	}
	return NULL;
}

void free_node(queue_node_t *node){
	node->is_free = 1;
	return;
}


__INLINE int svc_msg_send(msg_t *m, unsigned int target_pid, unsigned int block){
	int ret = 0;
	asm volatile("mov r0, %[message]": : [message] "r" (m)); 	/* copy message address	*/
	asm volatile("mov r1, %[pid]": : [pid] "r" (target_pid));		/* copy target pid		*/
	asm volatile("mov r2, %[block]": : [block] "r" (block)); 	/* copy block			*/
	asm volatile("svc #0x1");		/*	call svc	*/
	asm volatile("mov %[ret], r0": [ret] "=r" (ret));

	return ret;



}


int msg_send_svc(msg_t *m, unsigned int target_pid, unsigned int block)
{
    tcb_t *target = (tcb_t*) sched_threads[target_pid];

    m->sender_pid = thread_pid;

    if (m->sender_pid == target_pid) {
        return -1;
    }

    if (target == NULL) {
        return -1;
    }

    if (target->status != STATUS_RECEIVE_BLOCKED & block != 2) {
        if (target->msg_array && queue_msg(target, m)) {
        	if (target->status == STATUS_SLEEPING){
        	  	thread_wakeup(target_pid);
        	}
            return 1;
        }

        if (!block) {
            DEBUG("msg_send: %s: Receiver not waiting, block=%u\n", active_thread->name, block);
            return 0;
        }

        DEBUG("msg_send: %s: send_blocked.\n", active_thread->name);
        queue_node_t *n = get_node();
        n->priority = active_thread->priority;
        n->data = (unsigned int) active_thread;
        n->next = NULL;
        DEBUG("msg_send: %s: Adding node to msg_waiters:\n", active_thread->name);

        queue_priority_add(&(target->msg_waiters), n);

        active_thread->wait_data = m;

        int newstatus;

        if (active_thread->status == STATUS_REPLY_BLOCKED) {
            newstatus = STATUS_REPLY_BLOCKED;
        }
        else {
            newstatus = STATUS_SEND_BLOCKED;
            if (sched_threads[target_pid]->status == STATUS_SLEEPING){
            	thread_wakeup(target_pid);
            }
        }

        sched_set_status((tcb_t*) active_thread, newstatus);

        DEBUG("msg_send: %s: Back from send block.\n", active_thread->name);
    }else if ((block == 2) & (target->status != STATUS_REPLY_BLOCKED)) {
    	DEBUG("msg_reply(): %s: Target \"%s\" not waiting for reply.", active_thread->name, target->name);
    	return -1;
    }
    else {
    	copy_msg(m, target->wait_data);
    	sched_set_status(target, STATUS_PENDING);
    }


    thread_yield();

    return 1;
}

int msg_send(msg_t *m, unsigned int target_pid, bool block)
{
    if (inISR()) {
        return msg_send_int(m, target_pid);
    }

    tcb_t *target = (tcb_t*) sched_threads[target_pid];

    m->sender_pid = thread_pid;

    if (m->sender_pid == target_pid) {
        return -1;
    }

    if (target == NULL) {
        return -1;
    }

    dINT();

    if (target->status != STATUS_RECEIVE_BLOCKED) {
        if (target->msg_array && queue_msg(target, m)) {
            eINT();
            return 1;
        }

        if (!block) {
            DEBUG("msg_send: %s: Receiver not waiting, block=%u\n", active_thread->name, block);
            eINT();
            return 0;
        }

        DEBUG("msg_send: %s: send_blocked.\n", active_thread->name);
        queue_node_t n;
        n.priority = active_thread->priority;
        n.data = (unsigned int) active_thread;
        n.next = NULL;
        DEBUG("msg_send: %s: Adding node to msg_waiters:\n", active_thread->name);

        queue_priority_add(&(target->msg_waiters), &n);

        active_thread->wait_data = (void*) m;

        int newstatus;

        if (active_thread->status == STATUS_REPLY_BLOCKED) {
            newstatus = STATUS_REPLY_BLOCKED;
        }
        else {
            newstatus = STATUS_SEND_BLOCKED;
        }

        sched_set_status((tcb_t*) active_thread, newstatus);

        DEBUG("msg_send: %s: Back from send block.\n", active_thread->name);
    }
    else {
        DEBUG("msg_send: %s: Direct msg copy from %i to %i.\n", active_thread->name, thread_getpid(), target_pid);
        /* copy msg to target */
        msg_t *target_message = (msg_t*) target->wait_data;
        *target_message = *m;
        sched_set_status(target, STATUS_PENDING);
    }

    eINT();
    thread_yield();

    return 1;
}

int msg_send_int(msg_t *m, unsigned int target_pid)
{
    tcb_t *target = (tcb_t *) sched_threads[target_pid];

    if (target->status == STATUS_RECEIVE_BLOCKED) {
        DEBUG("msg_send_int: Direct msg copy from %i to %i.\n", thread_getpid(), target_pid);

        m->sender_pid = target_pid;

        /* copy msg to target */
        msg_t *target_message = (msg_t*) target->wait_data;
        *target_message = *m;
        sched_set_status(target, STATUS_PENDING);

        sched_context_switch_request = 1;
        return 1;
    }
    else {
        DEBUG("msg_send_int: Receiver not waiting.\n");
        return (queue_msg(target, m));
    }
}

int msg_send_receive(msg_t *m, msg_t *reply, unsigned int target_pid)
{
    dINT();
    tcb_t *me = (tcb_t*) sched_threads[thread_pid];
    sched_set_status(me, STATUS_REPLY_BLOCKED);
    me->wait_data = (void*) reply;

    /* msg_send blocks until reply received */

    return msg_send(m, target_pid, true);
}

__INLINE int svc_msg_send_recieve(msg_t *m, msg_t *reply,  unsigned int target_pid){
	tcb_t *me = (tcb_t*) sched_threads[thread_pid];
	asm volatile("mov r0, %[reply]": : [reply] "r" (reply)); 	/* copy message address	*/
	asm volatile("svc #0x2");		/*	call svc to set content-ptr	*/
	svc_sched_set_status(STATUS_REPLY_BLOCKED);

	/* msg_send blocks until reply received */

	return svc_msg_send(m, target_pid, true);
}

__INLINE void set_msg_content_ptr(char *ptr){
	active_thread->wait_data->content.ptr = ptr;
}

__INLINE int svc_msg_reply(msg_t *m, msg_t *reply)
{
    return svc_msg_send(m, m->sender_pid, 2);
}

int msg_reply(msg_t *m, msg_t *reply)
{
    int state = disableIRQ();

    tcb_t *target = (tcb_t*) sched_threads[m->sender_pid];

    if (!target) {
        DEBUG("msg_reply(): %s: Target \"%" PRIu16 "\" not existing...dropping msg!\n", active_thread->name, m->sender_pid);
        return -1;
    }

    if (target->status != STATUS_REPLY_BLOCKED) {
        DEBUG("msg_reply(): %s: Target \"%s\" not waiting for reply.", active_thread->name, target->name);
        restoreIRQ(state);
        return -1;
    }

    DEBUG("msg_reply(): %s: Direct msg copy.\n", active_thread->name);
    /* copy msg to target */
    msg_t *target_message = (msg_t*) target->wait_data;
    *target_message = *reply;
    sched_set_status(target, STATUS_PENDING);
    restoreIRQ(state);
    thread_yield();

    return 1;
}

int msg_reply_int(msg_t *m, msg_t *reply)
{
    tcb_t *target = (tcb_t*) sched_threads[m->sender_pid];

    if (target->status != STATUS_REPLY_BLOCKED) {
        DEBUG("msg_reply_int(): %s: Target \"%s\" not waiting for reply.", active_thread->name, target->name);
        return -1;
    }

    msg_t *target_message = (msg_t*) target->wait_data;
    *target_message = *reply;
    sched_set_status(target, STATUS_PENDING);
    sched_context_switch_request = 1;
    return 1;
}

__INLINE int svc_msg_try_receive(msg_t *m)
{
    return _svc_msg_receive(m, 0);
}

__INLINE int svc_msg_receive(msg_t *m)
{
    return _svc_msg_receive(m, 1);
}

__INLINE int _svc_msg_receive(msg_t *m, unsigned int block){
	int ret = 0;
	asm volatile("mov r0, %[message]": : [message] "r" (m)); 	/* copy message address	*/
	asm volatile("mov r1, %[block]": : [block] "r" (block)); 	/* copy block			*/
	asm volatile("svc #0x4");		/*	call svc	*/
	asm volatile("mov %[retv], r0": [retv] "=r" (ret)); 		/* save return value	*/

	return ret;

}

int msg_receive_svc(msg_t *m, unsigned int block)
{
    DEBUG("_msg_receive: %s: _msg_receive.\n", active_thread->name);

    tcb_t *me = active_thread;

    int queue_index = -1;

    if (me->msg_array) {
        queue_index = cib_get(&(me->msg_queue));
    }

    /* no message, fail */
    if ((!block) && (queue_index == -1)) {
        return -1;
    }

    if (queue_index >= 0) {
        DEBUG("_msg_receive: %s: _msg_receive(): We've got a queued message.\n", active_thread->name);
        *m = me->msg_array[queue_index];
    }
    else {
        me->wait_data = (void *) m;
    }

    queue_node_t *node = queue_remove_head(&(me->msg_waiters));

    if (node == NULL) {
        DEBUG("_msg_receive: %s: _msg_receive(): No thread in waiting list.\n", active_thread->name);

        if (queue_index < 0) {
            DEBUG("_msg_receive(): %s: No msg in queue. Going blocked.\n", active_thread->name);
            sched_set_status(me, STATUS_RECEIVE_BLOCKED);

            thread_yield();

            /* sender copied message */
        }

        return 1;
    }
    else {
        DEBUG("_msg_receive: %s: _msg_receive(): Waking up waiting thread.\n", active_thread->name);
        tcb_t *sender = (tcb_t*) node->data;
        free_node(node);

        if (queue_index >= 0) {
            /* We've already got a message from the queue. As there is a
             * waiter, take it's message into the just freed queue space.
             */
            m = &(me->msg_array[cib_put(&(me->msg_queue))]);
        }

        /* copy msg */
        msg_t *sender_msg = (msg_t*) sender->wait_data;
        copy_msg(sender_msg, m);

        /* remove sender from queue */
        sender->wait_data = NULL;
        sched_set_status(sender, STATUS_PENDING);

        return 1;
    }
}

void copy_msg(msg_t *src, msg_t *dst){
	unsigned int size = src->size;
	if (dst->size >= src-> size ){
		unsigned int i;
		char *dst_data = NULL;
		tcb_t *sender = (tcb_t *) sched_threads[src->sender_pid];
		tcb_t *reciever = (tcb_t *) sched_threads[dst->sender_pid];
		if (size > 0){
			char *source_data = src->content.ptr;
			char *target_data = dst->content.ptr;
			dst_data = target_data;
			for (i = 0; (i < size) & ((target_data + i) < (char *) reciever->memory->end_address) & (source_data + i < (char *) sender->memory->end_address); i++){
				*(target_data+i) = *(source_data+i);
			}
		}
		/* copy msg to target */
		*dst = *src;
		if (dst_data != NULL){
			dst->content.ptr = dst_data;
		}
	}
}

int msg_try_receive(msg_t *m)
{
    return _msg_receive(m, 0);
}

int msg_receive(msg_t *m)
{
    return _msg_receive(m, 1);
}

static int _msg_receive(msg_t *m, int block)
{
    dINT();
    DEBUG("_msg_receive: %s: _msg_receive.\n", active_thread->name);

    tcb_t *me = (tcb_t*) sched_threads[thread_pid];

    int queue_index = -1;

    if (me->msg_array) {
        queue_index = cib_get(&(me->msg_queue));
    }

    /* no message, fail */
    if ((!block) && (queue_index == -1)) {
        return -1;
    }

    if (queue_index >= 0) {
        DEBUG("_msg_receive: %s: _msg_receive(): We've got a queued message.\n", active_thread->name);
        *m = me->msg_array[queue_index];
    }
    else {
        me->wait_data = (void *) m;
    }

    queue_node_t *node = queue_remove_head(&(me->msg_waiters));

    if (node == NULL) {
        DEBUG("_msg_receive: %s: _msg_receive(): No thread in waiting list.\n", active_thread->name);

        if (queue_index < 0) {
            DEBUG("_msg_receive(): %s: No msg in queue. Going blocked.\n", active_thread->name);
            sched_set_status(me, STATUS_RECEIVE_BLOCKED);

            eINT();
            thread_yield();

            /* sender copied message */
        }

        return 1;
    }
    else {
        DEBUG("_msg_receive: %s: _msg_receive(): Waking up waiting thread.\n", active_thread->name);
        tcb_t *sender = (tcb_t*) node->data;

        if (queue_index >= 0) {
            /* We've already got a message from the queue. As there is a
             * waiter, take it's message into the just freed queue space.
             */
            m = &(me->msg_array[cib_put(&(me->msg_queue))]);
        }

        /* copy msg */
        msg_t *sender_msg = (msg_t*) sender->wait_data;
        *m = *sender_msg;

        /* remove sender from queue */
        sender->wait_data = NULL;
        sched_set_status(sender, STATUS_PENDING);

        eINT();
        return 1;
    }
}


__INLINE int svc_msg_init_queue(msg_t *array, int num){
	int ret = 0;
	asm volatile("mov r0, %[array]": : [array] "r" (array)); 	/* copy message address	*/
	asm volatile("mov r1, %[num]": : [num] "r" (num)); 	/* copy block			*/
	asm volatile("svc #0x5");		/*	call svc	*/
	asm volatile("mov %[retv], r0": [retv] "=r" (ret)); 		/* save return value	*/

	return ret;
}

int msg_init_queue(msg_t *array, int num)
{
    /* check if num is a power of two by comparing to its complement */
    if (num && (num & (num - 1)) == 0) {
        tcb_t *me = (tcb_t*) active_thread;
        me->msg_array = array;
        cib_init(&(me->msg_queue), num);
        return 0;
    }

    return -1;
}
