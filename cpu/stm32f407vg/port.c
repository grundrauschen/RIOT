/**
 * riot cpu specific port of kernel functions
 *
 * Copyright (C) 2013 Freie Universität Berlin
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @file
 * @author      Stefan Pfeiffer <pfeiffer@inf.fu-berlin.de>
 *
 */


#include "cpu.h"
#include "core_cmFunc.h"
#include <conf.h>
#include <memmgmt.h>
#include <mpu.h>
#include <msg.h>

#define PRIV_USER_MODE  	0x2
#define PRIV_THREAD_MODE    0x0

extern void fk_task_exit(void);
extern int msg_send_svc(msg_t *, unsigned int, unsigned int);
extern void set_msg_content_ptr(char *);
extern void sched_set_status_svc(unsigned int);
extern int msg_receive_svc(msg_t *, unsigned int);
extern int msg_init_queue(msg_t *, int);
extern int thread_create(int stacksize, char priority, int flags, void (*function) (void), const char *name);
extern thread_mem_violation(void);

unsigned int atomic_set_return(unsigned int* p, unsigned int uiVal) {
	//unsigned int cspr = disableIRQ();		//crashes
	dINT();
	unsigned int uiOldVal = *p;
	*p = uiVal;
	//restoreIRQ(cspr);						//crashes
	eINT();
	return uiOldVal;
}

void cpu_switch_context_exit(void){
    sched_run();
#ifdef USE_MPU
	/* enable all defined zones */
	enable_zones_and_mpu();
#endif
    sched_task_return();
}


void thread_yield(void) {
	//asm("svc 0x01\n");
	// set PendSV Bit
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void MemManage_Handler(void) {

	__ISB();
	__DSB();
	thread_mem_violation();
	//NVIC_SystemReset();
}

__attribute__((naked))void PendSV_Handler(void)
{
	save_context();
#ifdef USE_MPU
	/* disable MPU */
	disable_mpu_all_zones();
#endif
	/* call scheduler update fk_thread variable with pdc of next thread	 */
	asm("bl sched_run");
#ifdef USE_MPU
	/* enable all defined zones */
	enable_zones_and_mpu();
#endif
	/* the thread that has higest priority and is in PENDING state */
	restore_context();
}

/**
  * @brief  This function handles SVCall exception.
  *
  * Copied from book: The Definitive Guide to ARM Cortex-M3 and ARM Cortex M4 - Joseph Yiu
  *
  * @param  None
  * @retval None
  */
__attribute__((naked)) void SVC_Handler(void)
{
	asm("tst lr,#4"); 		/* Test bit 2 of EXC_RETURN	*/
	asm("ite eq");
	asm("mrseq r0, msp"); 	/* if 0, stacking used MSP, copy to R0	*/
	asm("mrsne r0, psp");	/* if 1, stacking used PSP, copy to R0	*/
	asm("b SVC_Handler_C");
}

void SVC_Handler_C(unsigned int *svc_args){
	uint8_t svc_number;
	uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3, stacked_r12; /*, stacked_lr, stacked_pc, stacked_xpsr */

	svc_number = ((char *) svc_args[6])[-2]; /* Memory[(Stacked PC)-2] */
	stacked_r0 = svc_args[0];
	stacked_r1 = svc_args[1];
	stacked_r2 = svc_args[2];
	stacked_r3 = svc_args[3];
	stacked_r12 = svc_args[4];
	/*
	stacked_lr = svc_args[5];
	stacked_pc = svc_args[6];
	stacked_xpsr = svc_args[7];
	 */

	switch(svc_number){
		case 0: break;
		/* send message 	*/
		case 1: svc_args[0] = msg_send_svc((void *) stacked_r0, stacked_r1, stacked_r2);
				break;
		/* set pointer of return payload	*/
		case 2: set_msg_content_ptr((char *)stacked_r0);
				break;
		/* set status of thread				*/
		case 3: sched_set_status_svc(stacked_r0);
				break;
		/* receive message	*/
		case 4: msg_receive_svc(stacked_r0, stacked_r1);
				break;
		/* initialize message storage	*/
		case 5: msg_init_queue(stacked_r0, stacked_r1);
				break;
		/* create thread			*/
		case 6: svc_args[0] = thread_create_desc(stacked_r0);
				break;
		/* thread sleeping and scheduling 	*/
		case 7: thread_sleep();
				break;
		/* context_switch_exit()*/
		case 8: thread_yield();
				break;
		default: break;
	}

}


// /* kernel functions */
//void ctx_switch(void)
//{
//	/* Save return address on stack */
//	/* stmfd   sp!, {lr} */
//
//	/* disable interrupts */
//	/* mov     lr, #NOINT|SVCMODE */
//	/* msr     CPSR_c, lr */
//	/* cpsid 	i */
//
//	ctx_switch2:
//	/* save other register */
//	asm("nop");
//
//	asm("mov r12, sp");
//	asm("stmfd r12!, {r4-r11}");
//
//	/* save user mode stack pointer in *fk_thread */
//	asm("ldr     r1, =active_thread"); /* r1 = &fk_thread */
//	asm("ldr     r1, [r1]"); /* r1 = *r1 = fk_thread */
//	asm("str     r12, [r1]"); /* store stack pointer in tasks pdc*/
//
//	sched_task_return();
//}

/* call scheduler so fk_thread points to the next task */
void sched_task_return(void)
{
	/* switch to user mode use PSP insteat of MSP in ISR Mode*/

//	unsigned int mode = PRIV_USER_MODE;
//
//	asm("msr CONTROL,%0 ": : "r" (mode):);
	CONTROL_Type mode;
	mode.w = __get_CONTROL();

	mode.b.SPSEL = 1; // select PSP
	/*mode.b.nPRIV = 0; // privilege*/

	__set_CONTROL(mode.w);

	/* load pdc->stackpointer in r0 */
	asm("ldr     r0, =active_thread"); /* r0 = &fk_thread */
	asm("ldr     r0, [r0]"); /* r0 = *r0 = fk_thread */
	asm("ldr     sp, [r0]"); /* sp = r0  restore stack pointer*/
	asm("pop		{r4}"); /* skip exception return */
	asm(" pop		{r4-r11}");
	asm(" pop		{r0-r3,r12,lr}"); /* simulate register restor from stack */
//	asm("pop 		{r4}"); /*foo*/
	asm("pop		{pc}");
}
/*
 * cortex m4 knows stacks and handles register backups
 *
 * so use different stack frame layout
 *
 *
 * with float storage
 * ------------------------------------------------------------------------------------------------------------------------------------
 * | R0 | R1 | R2 | R3 | LR | PC | xPSR | S0 | S1 | S2 | S3 | S4 | S5 | S6 | S7 | S8 | S9 | S10 | S11 | S12 | S13 | S14 | S15 | FPSCR |
 * ------------------------------------------------------------------------------------------------------------------------------------
 *
 * without
 *
 * --------------------------------------
 * | R0 | R1 | R2 | R3 | LR | PC | xPSR |
 * --------------------------------------
 *
 *
 */
char *thread_stack_init(void (*task_func)(void), void *stack_start, int stack_size)
{
	unsigned int * stk;
	stk = (unsigned int *)(stack_start );

	/* marker */
	stk--;
	*stk = 0x77777777;

	//FIXME FPSCR
	stk--;
	*stk = (unsigned int) 0;

	//S0 - S15
	for (int i = 15; i >= 0; i--) {
		stk--;
		*stk = i;
	}

	//FIXME xPSR
	stk--;
	*stk = (unsigned int) 0x01000200;

	//program counter
	stk--;
	*stk = (unsigned int) task_func;

	/* link register */
	stk--;
	*stk = (unsigned int) 0x0;

	/* r12 */
	stk--;
	*stk = (unsigned int) 0;

	/* r0 - r3 */
	for (int i = 3; i >= 0; i--) {
		stk--;
		*stk = i;
	}

	/* r11 - r4 */
	for (int i = 11; i >= 4; i--) {
		stk--;
		*stk = i;
	}

	/* lr means exception return code  */
	stk--;
	*stk = (unsigned int) 0xfffffffd; // return to taskmode main stack pointer

	return (char*) stk;
}


/*----------------------------------------------------------------------------------------*/
/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}



/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

void WWDG_IRQHandler(void){
	/* Go to infinite loop when Usage Fault exception occurs */
	  while (1)
	  {
	  }
}
