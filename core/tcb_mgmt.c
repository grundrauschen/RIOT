#include "tcb_mgmt.h"

/* *************************************** TCB MGMT ************************* */

void init_tcb_storage(void){
	int i;
	for (i=0; i<MAX_CON_THREADS; i++){
		tcb_storage[i].is_used = 0;
	}
}

tcb_t* get_tcb(void){
	int i;
	for (i=0; i<MAX_CON_THREADS;i++){
		if (tcb_storage[i].is_used == 0){
			tcb_storage[i].is_used = 1;
			return &tcb_storage[i];
		}
	}
	return NULL;
}

void free_tcb(tcb_t *tcb){
	tcb->is_used = 0;
}




