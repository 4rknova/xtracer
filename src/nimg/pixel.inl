/*

    This file is part of libimg.

	pixel.inl
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

#ifndef NIMG_PIXEL_INL_INCLUDED
#define NIMG_PIXEL_INL_INCLUDED

#ifndef NIMG_PIXEL_H_INCLUDED
	error "pixel.h must be included before pixel.inl"
#endif /* NIMG_PIXEL_H_INCLUDED */

namespace NImg {

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

static inline pixel32_t rgba_c_to_pixel32(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (pixel32_t)(((uint32_t)r << 24) + ((uint32_t)g << 16) + ((uint32_t)b << 8) + (uint32_t)a);
}

static inline pixel32_t rgba_f_to_pixel32(scalar_t r, scalar_t g, scalar_t b, scalar_t a)
{
	pixel32_t pix = 0;

	/* Do some basic tone mapping. */
	scalar_t fmax = 0.0f;
	fmax = r > fmax ? r : fmax;
	fmax = g > fmax ? g : fmax;
	fmax = b > fmax ? b : fmax;

	scalar_t scale = 1.0f;

	if (fmax > 1.0f) {
		scale = 1.f / fmax;
	}

	scalar_t cr = r * scale * 255.0f;
	scalar_t cg = g * scale * 255.0f;
	scalar_t cb = b * scale * 255.0f;

	pix = rgba_c_to_pixel32((char)cr, (char)cg, (char)cb, (char)ca);
	return pix;
}

static inline unsigned char get_pixel32_r(pixel32_t c)
{
	return (c & 0xFF000000) >> 24;
}

static inline unsigned char get_pixel32_g(pixel32_t c)
{
	return (c & 0x00FF0000) >> 16;
}

static inline unsigned char get_pixel32_b(pixel32_t c)
{
	return (c & 0x0000FF00) >> 8;
}

static inline unsigned char get_pixel32_a(pixel32_t c)
{
	return c & 0x000000FF;
}

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */

} /* namespace NImg */

#endif /* NIMG_PIXEL_INL_INCLUDED */
