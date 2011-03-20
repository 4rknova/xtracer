/*

	This file is part of xtracer.

	drv.hpp
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

#ifndef XTRACER_OUTPUT_HPP_INCLUDED
#define XTRACER_OUTPUT_HPP_INCLUDED

#include "err.h"
#include "fb.hpp"

enum XT_DRV_SYN
{
	XT_DRV_SYN_SYNC,
	XT_DRV_SYN_ASYN
};

enum XT_DRV
{
	XT_DRV_DUM,		/* Dummy driver */
	XT_DRV_SDL,   	/* Output to SDL window */
	XT_DRV_IMG,    	/* Output to image file */
	XT_DRV_ASC     	/* Output to stdout in ASCII */
};

#define XT_DRV_INTV unsigned int

/* Default sync mode */
#define XT_DEFAULT_DRV_SYN	XT_DRV_SYN_SYNC
#define XT_DEFAULT_DRV_INTV	5
/* Default output mode */
#define XT_DEFAULT_DRV XT_DRV_SDL

class Driver
{
	public:
		Driver(Framebuffer &fb, XT_DRV_INTV intv = XT_DEFAULT_DRV_INTV, XT_DRV_SYN sync = XT_DEFAULT_DRV_SYN);
		~Driver();

		XT_DRV_SYN sync();
		XT_DRV_INTV interval();
		
		XT_DRV_SYN set_sync(XT_DRV_SYN v);
		XT_DRV_INTV set_interval(XT_DRV_INTV v);

		virtual xt_status_t init();
		virtual xt_status_t deinit();

		virtual xt_status_t update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);
		virtual xt_status_t update();
		virtual xt_status_t flip();
		  
	protected:
		XT_DRV_SYN m_p_sync;
		XT_DRV_INTV m_p_interval;
		Framebuffer *m_p_fb;
};

#endif /* XTRACER_OUTPUT_HPP_INCLUDED */
