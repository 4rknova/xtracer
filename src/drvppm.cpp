/*

	This file is part of xtracer.

	ppm.cpp
	PPM driver

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

#include "drvppm.hpp"
#include "pixel.h"

DrvPPM::DrvPPM( Framebuffer &fb):
	Driver(fb)
{}

DrvPPM::~DrvPPM()
{}

unsigned int DrvPPM::init()
{
	return 0;
}

unsigned int DrvPPM::deinit()
{
	return 0;
}

#include <cstdio>
#include <iostream>
#include "pixel.h"

unsigned int DrvPPM::update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	// check for "out of bounds" and inverse mapping errors
	if ((x0 > m_fb->width()) 
		|| (x1 > m_fb->width())
		|| (y0 > m_fb->height())
		|| (y1 > m_fb->height())
		|| (x0 > x1)
		|| (y0 > y1))
		return 1;

	FILE *fp;

	std::string fname = m_fb->tag();

	fname.append(".ppm");

	std::cout << "Dumping framebuffer -> " << fname << "\n";
	if(!(fp = fopen(fname.c_str(), "wb")))
	{
		std::cerr << "Failed to open " << fname << " for output\n";
		return 1;
	}

	unsigned int width = x1 - x0;
	unsigned int height = y1 - y0;

	/*
		Header
		- magic number: P6
		- Whitespace (blanks, TABs, CRs, LFs).
		- A width, formatted as ASCII characters in decimal.
		- Whitespace.
		- A height, again in ASCII decimal.
		- Whitespace.
		- The maximum color value (Maxval), again in ASCII decimal.
		  Must be less than 65536 and more than zero.
		- A single whitespace character (usually a newline).
	*/
	fprintf(fp, "P6\n%d %d\n255\n", width, height);
	for(unsigned int y = y0; y < y1; y++)
	{
		for(unsigned int x = x0; x < x1; x++)
		{
			Vector3 pixel = *(m_fb->pixel(x, y));

			pixel32_t p = rgba_f_to_pixel32(pixel.x, pixel.y, pixel.z, 1.0);

			fputc(get_pixel32_r(p), fp);
			fputc(get_pixel32_g(p), fp);
			fputc(get_pixel32_b(p), fp);
		}
	}

	fclose(fp);

	return 0;
}

unsigned int DrvPPM::flip()
{
	return 0;
}

bool DrvPPM::is_realtime()
{
	return false;
}
