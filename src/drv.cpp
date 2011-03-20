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

Driver::Driver(Framebuffer &fb, XT_DRV_INTV intv, XT_DRV_SYN sync):
	m_p_sync(sync), m_p_interval(intv), m_p_fb(&fb)
{}

Driver::~Driver()
{}

XT_DRV_SYN Driver::sync()
{
	return m_p_sync;
}

XT_DRV_INTV Driver::interval()
{
	return m_p_interval;
}

XT_DRV_SYN Driver::set_sync(XT_DRV_SYN v)
{
	return m_p_sync = v;
}

XT_DRV_INTV Driver::set_interval(XT_DRV_INTV v)
{
	return m_p_interval = v;
}

xt_status_t Driver::init()
{
	return XT_STATUS_OK;
}

xt_status_t Driver::deinit()
{
	return XT_STATUS_OK;
}

xt_status_t Driver::update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	return XT_STATUS_OK;
}

xt_status_t Driver::update()
{
	return XT_STATUS_OK;
}

xt_status_t Driver::flip()
{
	return XT_STATUS_OK;
}
