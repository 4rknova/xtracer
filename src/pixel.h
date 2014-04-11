/*

    This file is part of libnimg.

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

#ifndef NIMG_PIXEL_H_INCLUDED
#define NIMG_PIXEL_H_INCLUDED

#include <stdint.h>
#include "precision.h"
#include "defs.h"

namespace NImg {

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

/*
	NOTES:
	All the functions below operate on a big endian format.
*/

typedef uint32_t pixel32_t;

static inline pixel32_t rgba_c_to_pixel32(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
static inline pixel32_t rgba_f_to_pixel32(scalar_t r, scalar_t g, scalar_t b, scalar_t a);

static inline unsigned char get_pixel32_r(pixel32_t c);
static inline unsigned char get_pixel32_g(pixel32_t c);
static inline unsigned char get_pixel32_b(pixel32_t c);
static inline unsigned char get_pixel32_a(pixel32_t c);

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */

} /* namespace NImg */

#include "pixel.inl"

#endif /* XTRACER_PIXEL_H_INCLUDED */
