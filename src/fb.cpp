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

XTFramebuffer::XTFramebuffer(unsigned int x, unsigned int y)
	: m_p_width(x > 0 ? x : 1), m_p_height(y > 0 ? y : 1)
{
	m_p_pixels = new uint32_t[x * y];
}

XTFramebuffer::~XTFramebuffer()
{
	delete [] m_p_pixels;
}

unsigned int XTFramebuffer:: get_width()
{
	return  m_p_width;
}

unsigned int XTFramebuffer:: get_height()
{
	return  m_p_height;
}

uint32_t XTFramebuffer::get_pixel(unsigned int x, unsigned int y)
{
	unsigned int n = y * m_p_width + x;
	return (x < m_p_width) ? ((y < m_p_height) ? m_p_pixels[n] : m_p_pixels[m_p_width * m_p_height - 1])  : m_p_pixels[m_p_width * m_p_height - 1];
}

uint32_t XTFramebuffer::set_pixel(unsigned int x, unsigned int y, uint32_t value)
{
	unsigned int n = y * m_p_width + x;
	uint32_t *target =(x < m_p_width) ? ((y < m_p_height) ? &m_p_pixels[n] : &m_p_pixels[m_p_width * m_p_height - 1])  : &m_p_pixels[m_p_width * m_p_height - 1];
	*target =  value;
	return value;
}
