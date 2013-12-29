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
		uint32_t HFNMIENA:1;		/*!< bit:	1		enables the MPU during Hardfault*/
		uint32_t PRIVDEFENA:1;	/*!< bit:	2		enables default memory map		*/
		uint32_t _reserved0:29;	/*!< bit: 3..31		Reserved						*/
	}b;							/*!< Structure used for bit access					*/
	uint32_t w;					/*!< Value used for word access						*/
}MPU_CTRL_Type;

/** \brief Union type to access the MPU.RNR Register
 *
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
		uint32_t SIZE:4;			/*!< bit	1..5	Size of Region -> formula	*/
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

#endif // _MEMORY_MGMT_H
