/*

    This file is part of xtracer.

   	pixel.h
    Pack and extract color components

    Copyright (C) 2010, 2011
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

#ifndef XTRACER_PIXEL_H_INCLUDED
#define XTRACER_PIXEL_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

/* Pack a rgba tuple to uint32  */
uint32_t rgba_to_uint32(unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha);

/* Get component from packed rgba */
unsigned int get_rgba_r(uint32_t color);
unsigned int get_rgba_g(uint32_t color);
unsigned int get_rgba_b(uint32_t color);
unsigned int get_rgba_a(uint32_t color);

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */

#endif /* XTRACER_PIXEL_H_INCLUDED  */
