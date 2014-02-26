/**
 * platform-independent kernel initialization
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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <tcb.h>
#include <kernel.h>
#include <kernel_internal.h>
#include <sched.h>
#include <flags.h>
#include <cpu.h>
#include <lpm.h>
#include <thread.h>
#include <memmgmt.h>
#include <tcb_mgmt.h>
#include <mpu.h>

#ifdef MODULE_AUTO_INIT
#include <auto_init.h>
#endif

volatile int lpm_prevent_sleep = 0;

extern int main(void);

static void idle_thread(void)
{
    while (1) {
        if (lpm_prevent_sleep) {
            lpm_set(LPM_IDLE);
        }
        else {
            lpm_set(LPM_IDLE);
            /* lpm_set(LPM_SLEEP); */
            /* lpm_set(LPM_POWERDOWN); */
        }
    }
}

const char *main_name = "main";
const char *idle_name = "idle";

static char main_stack[KERNEL_CONF_STACKSIZE_MAIN];
static char idle_stack[KERNEL_CONF_STACKSIZE_IDLE];

#ifdef MODULE_AUTO_INIT
#define MAIN_FUNC auto_init
#else
#define MAIN_FUNC ((void (*) (void))  main)
#endif



void kernel_init(void)
{
    dINT();
    printf("kernel_init(): This is RIOT! (Version: %s)\n", VERSION);



    sched_init();

    init_tcb_storage();

    init_memory_mgmt();

#ifdef USE_MPU
    enable_mpu_irc();
#endif

    if (thread_create(512, PRIORITY_IDLE, CREATE_WOUT_YIELD | CREATE_STACKTEST, idle_thread, idle_name) < 0) {
        printf("kernel_init(): error creating idle task.\n");
    }

    if (thread_create(8192, PRIORITY_MAIN, CREATE_WOUT_YIELD | CREATE_STACKTEST, MAIN_FUNC, main_name) < 0) {
        printf("kernel_init(): error creating main task.\n");
    }

    eINT();
    printf("kernel_init(): jumping into first task...\n");
    printf("SCB-MMFAR Address: %#010x\n", &SCB->MMFAR);

    cpu_switch_context_exit();
}

