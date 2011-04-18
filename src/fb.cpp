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

Framebuffer::Framebuffer(unsigned int width, unsigned int height): 
	m_tag(FB_DEFAULT_TAG),
	m_width(width > 0 ? width : 1), 
	m_height(height > 0 ? height : 1),
	m_pixels(new uint32_t[m_width * m_height])
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

uint32_t Framebuffer::get_pixel(unsigned int x, unsigned int y)
{
	unsigned int n = y * m_width + x;
	return (x < m_width) ? ((y < m_height) ? m_pixels[n] : m_pixels[m_width * m_height - 1])  : m_pixels[m_width * m_height - 1];
}

uint32_t Framebuffer::set_pixel(unsigned int x, unsigned int y, uint32_t value)
{
	unsigned int n = y * m_width + x;
	uint32_t *target =(x < m_width) ? ((y < m_height) ? &m_pixels[n] : &m_pixels[m_width * m_height - 1])  : &m_pixels[m_width * m_height - 1];
	*target =  value;
	return value;
}
