/*

    This file is part of xtracer.

    fb.hpp
    Framebuffer class

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

#ifndef XTRACER_FB_HPP_INCLUDED
#define XTRACER_FB_HPP_INCLUDED

#include <string>
#include "nmath/precision.h"
#include "nmath/vector.h"

#define FB_DEFAULT_TAG "framebuffer"

class Framebuffer
{
	public:
		Framebuffer(unsigned int width, unsigned int height);
		~Framebuffer();

		unsigned int width();
		unsigned int height();
		std::string &tag(const char *tag=NULL);

		void clear(Vector3 v = Vector3(0, 0, 0));	// Clear to color v

		// filters
		void apply_gamma(scalar_t v);			// Apply gamma correction
		void apply_exposure(scalar_t v);		// Apply exposure

		// Pixels are being accessed starting with (0,0). 
		// In case of out of bounds exceptions, the last pixel is affected.
		Vector3 *pixel(unsigned int x, unsigned int y);	

	private:
		std::string m_tag;

		const unsigned int m_width;
		const unsigned int m_height;

		Vector3 *m_pixels;
};

#endif /* XTRACER_FB_HPP_INCLUDED */
