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
#include <core_cm4.h>

#define ENABLE_DEBUG (1)
#include <debug.h>


