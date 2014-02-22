/*
 * tcb_mgmt.h
 *
 *  Created on: Feb 19, 2014
 *      Author: tobias
 */

#ifndef TCB_MGMT_H_
#define TCB_MGMT_H_

#include <stddef.h>
#include <stdio.h>
#include <conf.h>
#include <tcb.h>

/* ************************** TCP ************************************************ */

tcb_t tcb_storage[MAX_CON_THREADS];

void init_tcb_storage(void);
void tcb_free(tcb_t*);
tcb_t* get_tcb(void);

#endif /* TCB_MGMT_H_ */
