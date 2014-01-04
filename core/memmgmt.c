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

void  MemManage_Handler(void) {
	printf("Memory Protection Violation: naughty thread!");
}


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
