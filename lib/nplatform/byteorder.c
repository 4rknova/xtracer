/*

    This file is part of xtracer.

    byteorder.c
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

#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

short endian() /* returns the machine endianess code */
{
    union
    {
        uint32_t i;
        uint8_t c[4];
    } bint = {0x01020304};

    switch (bint.c[0])
    {
        case 0x01: return ENDIAN_BIG;
        case 0x04: return ENDIAN_LITTLE;
    }
    return ENDIAN_UNKNOWN;
}

#define ENDIAN_STRING_UNKNOWN   "Unknown"
#define ENDIAN_STRING_LITTLE    "Little"
#define ENDIAN_STRING_BIG       "Big"

char* endian_string()
{
    switch (endian())
    {
        case ENDIAN_BIG: return ENDIAN_STRING_BIG;
        case ENDIAN_LITTLE: return ENDIAN_STRING_LITTLE;
    }
    return ENDIAN_STRING_UNKNOWN;
}

#ifdef __cplusplus
} /* extern */
#endif /* __cplusplus */
