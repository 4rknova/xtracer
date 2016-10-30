/*

    This file is part of xtracer.

    byteorder.h
    Endianness related functions

    Copyright (C) 2008, 2010
    Papadopoulos Nikolaos

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifndef LIBNPLATFORM_BYTEORDER_H_INCLUDED
#define LIBNPLATFORM_BYTEORDER_H_INCLUDED

#include <stdint.h>

/*
    Only little and big endians are supported.
    If your target machine uses middle endian or varriant then this library is not for you.
*/

#define ENDIAN_UNKNOWN  0x00    /* unknown endian */
#define ENDIAN_LITTLE   0x01    /* little endian */
#define ENDIAN_BIG      0x02    /* big endian */

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

short endian();        /* returns the machine endian code */
char* endian_string(); /* returns the machine endian string */

/* Functions to swap the endian of 16 and 32 bit integers */
static __inline__ void uint16_swap_endian(uint16_t *val);
static __inline__ void uint32_swap_endian(uint32_t *val);

/* Functions to convert data in correct endian */

/*
    Notes:
    These functions are to be used when we read/write a uint16_t or a uint32_t from/to a stream
    and need to impose a specific endianness.
    Ex. Cases in which a file format specifies that a value inside the file is stored in big 
		endian format.
*/
static __inline__ void uint16_to_big_endian(uint16_t *val);
static __inline__ void uint32_to_big_endian(uint32_t *val);
static __inline__ void uint16_to_lil_endian(uint16_t *val);
static __inline__ void uint32_to_lil_endian(uint32_t *val);

static __inline__ void uint16_from_big_endian(uint16_t *val);
static __inline__ void uint32_from_big_endian(uint32_t *val);
static __inline__ void uint16_from_lil_endian(uint16_t *val);
static __inline__ void uint32_from_lil_endian(uint32_t *val);

#ifdef __cplusplus
	} /* extern */
#endif /* __cplusplus */

#include "byteorder.inl"

#endif /* LIBNPLATFORM_BYTEORDER_H_INCLUDED */
