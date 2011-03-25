/*

    This file is part of xtracer.

    byteorder.inl
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

#ifndef LIBNPLATFORM_BYTEORDER_INL_INCLUDED
#define LIBNPLATFORM_BYTEORDER_INL_INCLUDED

#ifndef LIBNPLATFORM_BYTEORDER_H_INCLUDED
    #error "byteorder.h must be included before byteorder.inl"
#endif /* LIBNPLATFORM_BYTEORDER_H_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Functions to swap the endian of 16 and 32 bit integers */
static __inline__ void uint16_swap_endian(uint16_t *val)
{
    *val = (*val<<8) | (*val>>8);
}

#include <stdio.h>
static __inline__ void uint32_swap_endian(uint32_t *val)
{
    *val = (*val<<24) | ((*val<<8) & 0x00ff0000) | ((*val>>8) & 0x0000ff00) | (*val>>24);
}

/*
    Functions to convert data in correct endian

    Notes
    -------
    These functions are to be used when we read/write a uint16_t or a uint32_t from/to a stream
    and need to impose a specific endianness.

    Examples

        A file format specifies that a value inside the file is stored in big endian format and 
		we want to switch an value to save it in the file
        A file format specifies that a value inside the file is stored in big endian format and 
		we want to switch an value after reading it from a file
*/

/*
    To x_endian
    val refers to a value generated in local endianess
*/

static __inline__ void uint16_to_big_endian(uint16_t *val)
{
    if(endian()!= ENDIAN_BIG)
        uint16_swap_endian(val);
}

static __inline__ void uint32_to_big_endian(uint32_t *val)
{
    if(endian()!= ENDIAN_BIG)
        uint32_swap_endian(val);
}

static __inline__ void uint16_to_lil_endian(uint16_t *val)
{
    if(endian()!= ENDIAN_LITTLE)
        uint16_swap_endian(val);
}

static __inline__ void uint32_to_lil_endian(uint32_t *val)
{
    if(endian()!= ENDIAN_LITTLE)
        uint32_swap_endian(val);
}

/*
    From x_endian
    val refers to a value generated in known endianess
*/

static __inline__ void uint16_from_big_endian(uint16_t *val)
{
    if(endian()!= ENDIAN_BIG)
        uint16_swap_endian(val);
}

static __inline__ void uint32_from_big_endian(uint32_t *val)
{
    if(endian()!= ENDIAN_BIG)
        uint32_swap_endian(val);
}

static __inline__ void uint16_from_lil_endian(uint16_t *val)
{
    if(endian()!= ENDIAN_LITTLE)
        uint16_swap_endian(val);
}

static __inline__ void uint32_from_lil_endian(uint32_t *val)
{
    if(endian()!= ENDIAN_LITTLE)
        uint32_swap_endian(val);
}


#ifdef __cplusplus
} /* __cplusplus */
#endif

#endif /* LIBNPLATFORM_BYTEORDER_INL_INCLUDED */
