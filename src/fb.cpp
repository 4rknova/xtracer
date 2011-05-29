/*

    This file is part of xtracer.

    fb.hpp
    Framebuffer class

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

#include "fb.hpp"
#include <cmath>

Framebuffer::Framebuffer(unsigned int width, unsigned int height): 
	m_tag(FB_DEFAULT_TAG),
	m_width(width > 0 ? width : 1), 
	m_height(height > 0 ? height : 1),
	m_pixels(new Vector3[m_width * m_height])
{}

Framebuffer::~Framebuffer()
{
	delete [] m_pixels;
}

unsigned int Framebuffer::width()
{
	return  m_width;
}

unsigned int Framebuffer::height()
{
	return  m_height;
}

std::string &Framebuffer::tag(const char *tag)
{
	if (tag)
		m_tag = tag;

	return m_tag;
}

Vector3 *Framebuffer::pixel(unsigned int x, unsigned int y)
{
	unsigned int n = y * m_width + x;
	return (x < m_width) ? ((y < m_height) ? &m_pixels[n] : &m_pixels[m_width * m_height - 1])  : &m_pixels[m_width * m_height - 1];
}

void Framebuffer::clear(Vector3 v)
{
	for (unsigned int y = 0; y < m_height; y++)
		for (unsigned int x = 0; x < m_width; x++)
		{
			Vector3 *p = pixel(x, y);
			*p = v;
		}
}

void Framebuffer::apply_gamma(real_t v)
{
	for (unsigned int y = 0; y < m_height; y++)
		for (unsigned int x = 0; x < m_width; x++)
		{
			Vector3 *p = pixel(x, y);
			p->x = pow(p->x, 1.0 / v);
			p->y = pow(p->y, 1.0 / v);
			p->z = pow(p->z, 1.0 / v);
		}
}

void Framebuffer::apply_exposure(real_t v)
{
	for (unsigned int y = 0; y < m_height; y++)
		for (unsigned int x = 0; x < m_width; x++)
		{
			Vector3 *p = pixel(x, y);

			// apply exposure
			p->x = 1.0f - expf(p->x * (-v));
			p->y = 1.0f - expf(p->y * (-v));
			p->z = 1.0f - expf(p->z * (-v));
		}
}
