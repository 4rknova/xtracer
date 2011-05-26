/*

	This file is part of xtracer.

	drv.cpp
	Output drivers

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

#include "drv.hpp"

Driver::Driver(Framebuffer &fb):
	m_fb(&fb)
{}

Driver::~Driver()
{}

unsigned int Driver::init()
{
	return 0;
}

unsigned int Driver::deinit()
{
	return 0;
}

unsigned int Driver::update()
{
	return update(0, 0, m_fb->width(), m_fb->height());
}

unsigned int Driver::update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	return 0;
}

unsigned int Driver::flip()
{
	return 0;
}

bool Driver::is_realtime()
{
	return true;
}
