/*

	This file is part of xtracer.

	drv.hpp
	Output drivers

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

#ifndef XTRACER_DRV_HPP_INCLUDED
#define XTRACER_DRV_HPP_INCLUDED

#include "fb.hpp"
#include "console.hpp"

enum XT_DRV
{
	XT_DRV_DUM,		/* Dummy driver */
	XT_DRV_SDL,   	/* Output to SDL window */
	XT_DRV_PPM		/* Output to PPM image file */
};

/* Default output mode */
#define XT_DEFAULT_DRV XT_DRV_SDL

class Driver
{
	public:
		Driver(Framebuffer &fb, Console &con);
		~Driver();

		virtual unsigned int init();
		virtual unsigned int deinit();

		unsigned int update();
		virtual unsigned int update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);
		virtual unsigned int flip();

		virtual unsigned int hint();
		
		virtual bool is_realtime();
		  
	protected:
		Framebuffer *m_fb;
		Console *m_con;
};

#endif /* XTRACER_DRV_HPP_INCLUDED */
