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
#include "tcb.h"
#include <bitarithm.h>
#include <cpu.h>

/** \brief Union type to access the MPU.TYPE Register
 *
 */
typedef union {
	struct {
		uint32_t SEPERATE:1;		/*!< bit:	   0	seperate or unified memory map	*/
		uint32_t _reserved0:7;	/*!< bit: 	1..7 	Reserved 						*/
		uint32_t DREGION:8;		/*!< bit:	8..15	DREGION shows number of Regions */
		uint32_t IREGION:8;		/*!< bit:	16..23	IREGION number of Instuction Reg.*/
		uint32_t _reserved1:8;	/*!< bit:	24..31	Reserved						*/
	}b;							/*!< Structure used for bit access					*/
	uint32_t w;					/*!< Value used for word access						*/
}MPU_TYPE_Type;

/**	\brief Union type to access the MPU.CTRL Register
 *
 */
typedef union {
	struct {
		uint32_t ENABLE:1;		/*!< bit:	0		enables the MPU					*/
		uint32_t HFNMIENA:1;	/*!< bit:	1		enables the MPU during Hardfault*/
		uint32_t PRIVDEFENA:1;	/*!< bit:	2		enables default memory map		*/
		uint32_t _reserved0:29;	/*!< bit: 3..31		Reserved						*/
	}b;							/*!< Structure used for bit access					*/
	uint32_t w;					/*!< Value used for word access						*/
}MPU_CTRL_Type;


/** \brief Union type to access the MPU.RNR Register
 *
 * TODO: REGION als uint8_t definieren?
 */
typedef union {
	struct {
		uint32_t REGION:8;		/*!< bit:	0..7	Defines the Region number to protect	*/
		uint32_t _reserved0:24;	/*!< bit: 	8..31	Reserved								*/
	}b;							/*!< Structure used for bit access					*/
	uint32_t w;					/*!< Value used for word access						*/
}MPU_RNR_Type;

/** \brief Union type to access the MPU.RBAR Register
 *
 */
typedef union {
	struct {
		uint32_t REGION:4;		/*!< bit: 	0..3	sets the Region number		*/
		uint32_t VALID:1;		/*!< bit:	4		overrides the Region in RNR	*/
		uint32_t ADDRESS:27;		/*!< bit:	5..31	base adress, has to be aligned	*/
	}b;							/*!< Structure used for bit access					*/
	uint32_t w;					/*!< Value used for word access						*/
}MPU_RBAR_Type;

/** \brief Union type to access the MPU.RASR Register
 *
 */
typedef union {
	struct {
		uint32_t ENABLE:1;		/*!< bit 	0		enables Region				*/
		uint32_t SIZE:5;		/*!< bit	1..5	Size of Region -> formula	*/
		uint32_t _reserved0:2;	/*!< bit	6..7	Reserved					*/
		uint32_t SRD:8;			/*!< bit	8..15	Subregion Disable Bit		*/
		uint32_t B:1;			/*!< bit	16		one memory access attribute	*/
		uint32_t C:1;			/*!< bit	17		one memory access attribute	*/
		uint32_t S:1;			/*!< bit	18		Sharable bit				*/
		uint32_t TEX:3;			/*!< bit	19..21	memory access attribute		*/
		uint32_t _reserved1:2;	/*!< bit	22..23	Reserved					*/
		uint32_t AP:3;			/*!< bit	24..26	Access Permission			*/
		uint32_t _reserved2:1;	/*!< bit	27		Reserved					*/
		uint32_t XN:1;			/*!< bit	28		Instruction Fetch disable bit */
		uint32_t _reserved3:3;	/*!< bit	29..31	Reserved					*/
	}b;							/*!< Structure used for bit access					*/
	uint32_t w;					/*!< Value used for word access						*/
}MPU_RASR_Type;

#define MPU_CTRL	((MPU_CTRL_Type *) &MPU->CTRL)
#define MPU_TYPE	((MPU_TYPE_Type *) &MPU->TYPE)
#define MPU_RNR 	((MPU_RNR_Type *) &MPU->RNR)
#define MPU_RBAR 	((MPU_RBAR_Type *) &MPU->RBAR)
#define MPU_RASR	((MPU_RASR_Type *) &MPU->RASR)


/**
 * Handler routine for MPU-Interrupt
 */
/*void MemManage_Handler(void);*/

/**
 * Enables the MPU
 */
__STATIC_INLINE void __enable_MPU(void);

/**
 * Disables the MPU
 */
__STATIC_INLINE void __disable_MPU(void);


/**
 * Enables MPU during Hardfault
 */
__STATIC_INLINE void __enable_HFNMIENA(void);


/**
 * Disables MPU during Hardfault
 */
__STATIC_INLINE void __disable_HFNMIENA(void);

/**
 * Enables the Default Memory Map
 */
__STATIC_INLINE void __enable_PRIVDEFENA(void);

/**
 * Disables the Default Memory Map
 */
__STATIC_INLINE void __disable_PRIVDEFENA(void);

/**
 * Sets the REGION through the RBAR register
 *
 * @param[in]	region	Region Number (0-7)
 */
__STATIC_INLINE void __set_REGION_explicit(uint32_t region);

/**
 * Sets start address for a region
 *
 * @param[in]	rbar_reg	prepared MPU_RBAR_Type
 */
__STATIC_INLINE	void __set_Region(MPU_RBAR_Type *rbar_reg);

/**
 * Sets parameter of a region
 *
 * @param[in]	rasr_reg	prepared MPU_RASR_Type
 */
__STATIC_INLINE	void __set_RASR(MPU_RASR_Type *rasr_reg);

/**
 * Enables the MPU and protects the Area
 */
void enable_and_secure_MPU(uint32_t *start_pointer, uint32_t size, uint32_t region);

/**
 * Sets threads to unprivileged
 */
void enable_unprivileged_mode(void);

#endif // _MEMORY_MGMT_H
