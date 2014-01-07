/**
 * The RIOT scheduler implementation
 *
 * Copyright (C) 2013 Tobias Famulla
 *
 * @ingroup kernel
 * @{
 * @file
 * @author Tobias Famulla <def@famulla.eu>
 * @}
 */

#include <stdint.h>
#include <memmgmt.h>
#include <kernel.h>
#include <bitarithm.h>
#include <cpu.h>

#define ENABLE_DEBUG (1)
#include <debug.h>

/*
MPU_CTRL_Type *mpu_ctrl = (MPU_CTRL_Type *) MPU->CTRL;
volatile MPU_TYPE_Type *mpu_type = (MPU_TYPE_Type *) MPU->TYPE;
volatile MPU_RNR_Type *mpu_rnr = (MPU_RNR_Type *) MPU->RNR;
volatile MPU_RBAR_Type *mpu_rbar = (MPU_RBAR_Type *) MPU->RBAR;
volatile MPU_RASR_Type *mpu_rasr = (MPU_RASR_Type *) MPU->RASR;
*/

/*
void __attribute__(( naked )) MemManage_Handler(void) {
	printf("Memory Protection Violation: naughty thread!");
}
*/

__STATIC_INLINE void __enable_MPU(void) {
	MPU_CTRL->b.ENABLE = 1;
}

__STATIC_INLINE void __disable_MPU(void) {
	MPU_CTRL->b.ENABLE = 0;
}

__STATIC_INLINE void __enable_HFNMIENA(void) {
	MPU_CTRL->b.HFNMIENA = 1;
}

__STATIC_INLINE void __disable_HFNMIENA(void) {
	MPU_CTRL->b.HFNMIENA = 0;
}

__STATIC_INLINE void __enable_PRIVDEFENA(void) {
	MPU_CTRL->b.PRIVDEFENA = 1;
}

__STATIC_INLINE void __disable_PRIVDEFENA(void) {
	MPU_CTRL->b.PRIVDEFENA = 0;
}

__STATIC_INLINE void __set_REGION_explicit(uint32_t region) {
	MPU_RNR->b.REGION = region;
}

__STATIC_INLINE	void __set_Region(MPU_RBAR_Type *rbar_reg) {
	*MPU_RBAR = *rbar_reg;
}

__STATIC_INLINE	void __set_RASR(MPU_RASR_Type *rasr_reg) {
	*MPU_RASR = *rasr_reg;
}

void enable_and_secure_MPU(uint32_t *start_pointer, uint32_t size, uint32_t region){
	uint32_t start = (uint32_t) start_pointer;
	DEBUG("MPU ---------------\nStartpointer to protect: %#010x\n", start);
	/* We save 512 Bytes = n
	 * n = 2^(size + 1)
	 *
	 * start has to be aligned
	 * TODO: Muss später gesichert werden, hier nur testweise. (unsicher!)
	 * start uses bit 31:N N=log2 (n)
	 * somit N=size+1
	 */
	start = start >> 5;
	start = start & (0xFFFFFFFF << (size - 4));
	DEBUG("aligned startpointer: %#010x\n", start);

	MPU_RBAR_Type temp_rbar;
	temp_rbar.w = 0;
	temp_rbar.b.REGION = region;
	temp_rbar.b.VALID = 1;
	temp_rbar.b.ADDRESS = start;

	MPU_RASR_Type temp_rasr;
	temp_rasr.w = 0;
	temp_rasr.b.SIZE = size;
	temp_rasr.b.ENABLE = 1;
	temp_rasr.b.SRD = 0; /* no subregions disabled */
	temp_rasr.b.B = 0; /* according to book */
	temp_rasr.b.C = 1; /* according to book */
	temp_rasr.b.S = 1; /* according to book */
	temp_rasr.b.TEX = 0b000; /* according to book */
	temp_rasr.b.XN = 1; /* Instruction fetch forbidden */
	temp_rasr.b.AP = 0b110; /* RW for privileged and unprivileged */

	DEBUG("temp_rbar: %#010x\n", temp_rbar.w);
	DEBUG("temp_rasr: %#010x\n", temp_rasr.w);

	__DSB(); /* Memory barriers */
	__ISB();

	MPU_RBAR->w = temp_rbar.w;
	MPU_RASR->w = temp_rasr.w;
	DEBUG("Address of MPU_RBAR: %#010x\n", &MPU_RBAR->w);
	DEBUG("Address of MPU_RASR: %#010x\n", &MPU_RASR->w);
	DEBUG("MPU_RBAR: %#010x\n", MPU_RBAR->w);
	DEBUG("MPU_RASR: %#010x\n", MPU_RASR->w);


	__enable_PRIVDEFENA();
	__disable_HFNMIENA();
	/*	NVIC_EnableIRQ (MemoryManagement_IRQn);	Wrong !	Do not use this for cortex Processor exceptions
	 * Look at Source Code from "The Designers Guide to the Cortex-M Processor Family
	 */
		SCB->SHCSR = 0x01<<16;							/*The Fault exceptions are enabled in the System Control Block*/

	__enable_MPU();
	DEBUG("MPU Enabled ------\n");
	DEBUG("MPU_CTRL: %#010x\n", MPU_CTRL->w);
	DEBUG("MPU_RNR: %#010x\n", MPU_RNR->w);

}

void enable_unprivileged_mode(void) {
	CONTROL_Type mode;
	mode.w = __get_CONTROL();

	mode.b.SPSEL = 1; // select PSP
	mode.b.nPRIV = 1; // privilege

	__set_CONTROL(mode.w);
}
