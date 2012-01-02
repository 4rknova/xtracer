/*

    This file is part of xtracer.

   	pixel.h
    Pack and extract color components

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifndef XTRACER_PIXEL_H_INCLUDED
#define XTRACER_PIXEL_H_INCLUDED

#include <stdint.h> 

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

typedef uint32_t pixel32_t;

pixel32_t rgba_c_to_pixel32(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
pixel32_t rgba_f_to_pixel32(double red, double green, double blue, double alpha);

unsigned char get_pixel32_r(pixel32_t color);
unsigned char get_pixel32_g(pixel32_t color);
unsigned char get_pixel32_b(pixel32_t color);
unsigned char get_pixel32_a(pixel32_t color);

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */

#endif /* XTRACER_PIXEL_H_INCLUDED */
