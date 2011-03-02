/*

	This file is part of xtracer.

	out.hpp
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

#include "fb.hpp"

enum XTRACER_MODE_SYN
{
	XTRACER_SYN_SYNC,
	XTRACER_SYN_ASYN
};

/* Default sync mode */
#define XTRACER_DEFAULT_MODE_SYN XTRACER_SYN_SYNC
#define XTRACER_DEFAULT_ASYN_INTV	5

enum XTRACER_MODE_DRV
{
	XTRACER_DRV_SDL,        /* Output to SDL window */
	XTRACER_DRV_IMAGE,      /* Output to image file */
	XTRACER_DRV_ASCII       /* Output to stdout in ASCII */
};  

/* Default output mode */
#define XTRACER_DEFAULT_MODE_DRV XTRACER_DRV_SDL

/* Output environment */
struct Xtracer_out_env_t
{
	XTRACER_MODE_SYN syn;
	unsigned int syn_intv;
	XTRACER_MODE_DRV drv;
};

void out_init();						/* Initiate the output environment */
void out_set_syn(XTRACER_MODE_SYN m);	/* Set sync mode */
void out_set_syn_intv(unsigned int v);	/* Set async interval */
void out_set_drv(XTRACER_MODE_DRV d);	/* Set driver */


#endif /* XTRACER_OUTPUT_HPP_INCLUDED */
