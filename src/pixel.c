/*

    This file is part of xtracer.

   	pixel.c
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

#include "pixel.h"

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

pixel32_t rgba_c_to_pixel32(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
	return (pixel32_t)(((uint32_t)red << 24) + ((uint32_t)green << 16) + ((uint32_t)blue << 8) + (uint32_t)alpha);
}

pixel32_t rgba_f_to_pixel32(double red, double green, double blue, double alpha)
{
	/* correct the color */
	double r = red > 1.0 ? 1.0 : red;
	double g = green > 1.0 ? 1.0 : green;
	double b = blue > 1.0 ? 1.0 : blue;
	double a = alpha > 1.0 ? 1.0 : alpha;

	/* convert to pixel and update the framebuffer */
	r *= 255;
	g *= 255;
	b *= 255;
	a *= 255;

	pixel32_t pix = rgba_c_to_pixel32((char)r, (char)g, (char)b, (char)a);
	return pix;
}


unsigned char get_pixel32_r(pixel32_t color)
{
	return (color & 0xFF000000) >> 24;
}

unsigned char get_pixel32_g(pixel32_t color)
{
	return (color & 0x00FF0000) >> 16;
}

unsigned char get_pixel32_b(pixel32_t color)
{
	return (color & 0x0000FF00) >> 8;
}

unsigned char get_pixel32_a(pixel32_t color)
{
	return color & 0x000000FF;
}

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */
