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
#include <math.h>

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


memory_block_Type* add_block(memory_block_Type *current_block , uint32_t *block_start_address, uint32_t *block_end_address ){
	if (current_block->start_address == block_start_address){
		/* same start address */
		if (current_block->next_block->start_address == block_end_address + 4){
			/* same block */
			current_block->is_free = 0;
			return current_block;
		}
		else {
			memory_block_Type *next_block = get_free_struct();
			next_block->end_address = current_block->end_address;
			next_block->next_block = current_block->next_block;
			next_block->start_address = block_end_address + 4;
			next_block->is_free = 1;
			current_block->end_address = block_end_address;
			current_block->is_free = 0;
			return current_block;
		}
	}
	else {
		if (current_block->next_block != NULL && current_block->next_block->start_address == block_end_address + 4){
			memory_block_Type *next_block = get_free_struct();
			next_block->end_address = block_end_address;
			next_block->start_address = block_start_address;
			next_block->next_block = current_block->next_block;
			next_block->is_free = 0;
			current_block->end_address = block_start_address - 4;
			current_block->next_block = next_block;
			return next_block;
		}
		else {
			memory_block_Type *next_block = get_free_struct();
			memory_block_Type *after_block = get_free_struct();
			after_block->next_block = current_block->next_block;
			after_block->is_free = 1;
			after_block->end_address = current_block->end_address;
			after_block->start_address = block_end_address + 4;
			next_block->next_block = after_block;
			next_block->end_address = block_end_address;
			next_block->start_address = block_start_address;
			next_block->is_free = 0;
			current_block->next_block = next_block;
			current_block->end_address = block_start_address - 4;
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
					uint32_t *end_address = this_block->start_address + (size-1) - 3; /* aligned endadress */
					return add_block(this_block, this_block->start_address, end_address);
				}
				else {
					/* align address */
					uint32_t temp_address;
					temp_address = (((uint32_t) this_block->start_address) & ((size - 1) ^ 0xffffffff )) + size;
					if ((((uint32_t) this_block->end_address) - temp_address) > size){
						uint32_t *end_address = (uint32_t *) (temp_address + (size-1) - 3); /* aligned endadress */
						return add_block(this_block, (uint32_t *)temp_address, end_address);
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

/* **************************** MEM_PROP ************************************ */

uint32_t calculate_size(uint32_t *size){
	if(ispowerof2(*size)){
		return log(*size) / log(2) - 1;
	}
	return 0;
}

unsigned int init_mem_prop(mem_block_prop prop[], memory_block_Type *stack){
	/* Access to all memory parts -- only for debugging */
	uint32_t n = MEMSIZE;
	prop[0].AP = RW_RW;
	prop[0].XN = 0;
	prop[0].size = 256;
	prop[0].start_address = 0;
	/* safe stacks */
	prop[1].AP = RW_RO;
	prop[1].XN = 1;
	prop[1].size = calculate_size(&n);
	prop[1].start_address = first_mem_block->start_address;
	/* make thread stack */
	prop[2].AP = RW_RW;
	prop[2].XN = 1;
	uint32_t size = ((uint32_t) stack->end_address) - ((uint32_t) stack->start_address);
	prop[2].size = calculate_size(&size);
	prop[2].start_address = stack->start_address;
	return 3;

}
