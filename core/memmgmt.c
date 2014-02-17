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

extern const uint32_t user_stack_end;
extern const uint32_t user_stack_start;

static uint32_t mgmt_mem_start = (uint32_t) &user_stack_start;
static uint32_t mgmt_mem_end = (uint32_t) &user_stack_end;
static int mem_initialized = 0;

int ispowerof2(uint32_t);

void init_memory_mgmt(void) {
	first_mem_block = &block_array[0];
	first_mem_block->block_used = 1;
	first_mem_block->end_address = (uint32_t *) mgmt_mem_end;
	first_mem_block->start_address = (uint32_t *) mgmt_mem_start;
	first_mem_block->next_block = NULL;
	first_mem_block->is_free = 1;
	uint32_t i;
	for (i=1; i<BLOCKCOUNT; i++){
		block_array[i].block_used = 0;
		block_array[i].id = i;
	}
	mem_initialized = 1;
}

static memory_block_Type* get_free_struct(void){
	int i;
	for (i=0; i<BLOCKCOUNT;i++){
		if (block_array[i].block_used == 0){
			block_array[i].block_used = 1;
			return &block_array[i];
		}
	}
	return NULL;
}

__INLINE static void set_free_struct(int s){
	block_array[s].block_used = 0;
}

/** \brief calculates wheather x is power of 2 with bitmagic
 *
 * from: https://stackoverflow.com/questions/3638431/determine-if-an-int-is-a-power-of-2-or-not-in-a-single-line
 *
 */
__INLINE int ispowerof2(uint32_t x){
	return x && !(x & (x-1));
}


memory_block_Type* add_block(memory_block_Type *current_block , uint8_t *block_start_address, uint8_t *block_end_address ){
	if (current_block->start_address == block_start_address){
		/* same start address */
		if (current_block->next_block->start_address == block_end_address + 1){
			/* same block */
			current_block->is_free = 0;
			return current_block;
		}
		else {
			memory_block_Type *next_block = get_free_struct();
			next_block->end_address = current_block->end_address;
			next_block->next_block = current_block->next_block;
			next_block->start_address = block_end_address + 1;
			next_block->is_free = 1;
			current_block->end_address = block_end_address;
			current_block->is_free = 0;
			return current_block;
		}
	}
	else {
		if (current_block->next_block != NULL && current_block->next_block->start_address == block_end_address + 1){
			memory_block_Type *next_block = get_free_struct();
			next_block->end_address = block_end_address;
			next_block->start_address = block_start_address;
			next_block->next_block = current_block->next_block;
			next_block->is_free = 0;
			current_block->end_address = block_start_address - 1;
			current_block->next_block = next_block;
			return next_block;
		}
		else {
			memory_block_Type *next_block = get_free_struct();
			memory_block_Type *after_block = get_free_struct();
			after_block->next_block = current_block->next_block;
			after_block->is_free = 1;
			after_block->end_address = current_block->end_address;
			after_block->start_address = block_end_address + 1;
			next_block->next_block = after_block;
			next_block->end_address = block_end_address;
			next_block->start_address = block_start_address;
			next_block->is_free = 0;
			current_block->next_block = next_block;
			current_block->end_address = block_start_address - 1;
			return next_block;
		}
	}
	return NULL;
}

memory_block_Type* create_mem_block(uint32_t size){
	memory_block_Type *this_block;
	unsigned int further_block = 0;
	if (mem_initialized){
		this_block = first_mem_block;
		further_block = 1;
	}
	else {
		return NULL;
	}
	if (ispowerof2(size)) {
		while (further_block) {
			if (this_block->is_free && (((uint32_t) this_block->end_address - (uint32_t) this_block->start_address) > size )){
				/* if it fits, it sits */
				if (((uint32_t) this_block->start_address & (size - 1)) == 0 ){
					uint8_t *end_address = this_block->start_address + (size-1);
					return add_block(this_block, this_block->start_address, end_address);
				}
				else {
					/* align address */
					uint32_t temp_address;
					temp_address = (((uint32_t) this_block->start_address) & ((size - 1) ^ 0xffffffff )) + size;
					if ((((uint32_t) this_block->end_address) - temp_address) > size){
						uint8_t *end_address = (uint8_t *) (temp_address + (size-1));
						return add_block(this_block, (uint8_t *)temp_address, end_address);
					}
				}
			}
			if (this_block->next_block == NULL){
				further_block = 0;
			}
			else {
				this_block = this_block->next_block;
			}
		}
	}
	return NULL;
}



void free_mem_block(memory_block_Type *this_block){
	memory_block_Type *i_block = first_mem_block;
	while (i_block->next_block != this_block){
		i_block = i_block->next_block;
	}
	if (i_block != NULL){
		if (this_block->next_block->is_free == 1){
			this_block->end_address = this_block->next_block->end_address;
			this_block->next_block = this_block->next_block->next_block;
			this_block->is_free = 1;
			set_free_struct(this_block->next_block->id);
		}
		else if (i_block->is_free == 1){
			i_block->next_block = this_block->next_block;
			i_block->end_address = this_block->end_address;
			set_free_struct(this_block->id);
		}
		else {
			this_block->is_free = 1;
		}
	}
}


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
	temp_rasr.b.AP = 0b001; /* RW for privileged and unprivileged */

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
	/**
	 * Set Primask for interrupts
	 */
	/*__set_PRIMASK(0x0);*/

	/*	NVIC_EnableIRQ (MemoryManagement_IRQn);	Wrong !	Do not use this for cortex Processor exceptions
	 * Look at Source Code from "The Designers Guide to the Cortex-M Processor Family
	 */
	SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;							/*The Fault exceptions are enabled in the System Control Block*/

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
