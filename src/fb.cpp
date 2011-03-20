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

Framebuffer::Framebuffer(unsigned int width, unsigned int height, unsigned int bpp): 
	m_p_width(width > 0 ? width : XT_DEFAULT_FB_WIDTH), 
	m_p_height(height > 0 ? height : XT_DEFAULT_FB_HEIGHT),
	m_p_bpp(bpp > 0 ? bpp : XT_DEFAULT_FB_BPP),
	m_p_pixels(new uint32_t[width * height])
{}

Framebuffer::~Framebuffer()
{
	delete [] m_p_pixels;
}

unsigned int Framebuffer::width()
{
	return  m_p_width;
}

unsigned int Framebuffer::height()
{
	return  m_p_height;
}

unsigned int Framebuffer::bpp()
{
	return  m_p_bpp;
}

uint32_t Framebuffer::get_pixel(unsigned int x, unsigned int y)
{
	unsigned int n = y * m_p_width + x;
	return (x < m_p_width) ? ((y < m_p_height) ? m_p_pixels[n] : m_p_pixels[m_p_width * m_p_height - 1])  : m_p_pixels[m_p_width * m_p_height - 1];
}

uint32_t Framebuffer::set_pixel(unsigned int x, unsigned int y, uint32_t value)
{
	unsigned int n = y * m_p_width + x;
	uint32_t *target =(x < m_p_width) ? ((y < m_p_height) ? &m_p_pixels[n] : &m_p_pixels[m_p_width * m_p_height - 1])  : &m_p_pixels[m_p_width * m_p_height - 1];
	*target =  value;
	return value;
}

