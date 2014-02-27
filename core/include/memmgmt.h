/**
 * @ingroup kernel
 * @{
 * @file        memmgmt.h
 * @author      Tobias Famulla <dev@famulla.eu>
 */

#ifndef _MEMORY_MGMT_H
#define _MEMORY_MGMT_H

#include <stddef.h>
#include <bitarithm.h>
#include <bitarithm.h>
#include <cpu.h>
#include <conf.h>

#define NOACCESS 	0
#define RW_NO 		1
#define RW_RO		2
#define RW_RW		3
#define RO_NO		5
#define RO_RO		6

#define MAX_ZONES	8

#define MEMSIZE 32768

#define BLOCKCOUNT MAX_CON_THREADS

typedef struct mem_block_prop {
	uint32_t *start_address; 	/*!< startaddress to protect 	*/
	uint32_t size;				/*!< size of protected block 	*/
	uint16_t AP;				/*!< Access Permission			*/
	uint16_t XN;				/*!< No Access					*/

}mem_block_prop;


typedef struct memory_block memory_block_Type;

/** \brief management type for memory blocks
 *
 */
struct memory_block{
	memory_block_Type *next_block;		/* Pointer to next memory block */
	uint32_t block_used;		/* block in use					*/
	uint32_t is_free;			/* shows wheather block is free */
	uint32_t *start_address;	/* Begin of the block			*/
	uint32_t *end_address;		/* End of the block				*/
	uint32_t id;				/* id of Block					*/
};

static memory_block_Type *first_mem_block;
static memory_block_Type block_array[BLOCKCOUNT];

uint32_t align_pointer(uint32_t old_pointer, unsigned int size);

uint32_t align_pointer_next(uint32_t old_pointer, unsigned int size);

void init_memory_mgmt(void);
memory_block_Type* create_mem_block(uint32_t);
void free_mem_block(memory_block_Type *);
unsigned int init_mem_prop(mem_block_prop[], memory_block_Type*);

uint32_t calculate_size(uint32_t *size);


#endif // _MEMORY_MGMT_H
