/*

	This file is part of xtracer.

	out.cpp
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

#include "out.h"
#include "err.h"

/* Include the drivers */
#include "drv/dum.h"
#include "drv/sdl.h"

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

struct Xtracer_out_env_t xtracer_out_env;

void out_init()
{
	out_set_syn(XTRACER_DEFAULT_MODE_SYN);
	out_set_syn_intv(XTRACER_DEFAULT_ASYN_INTV);
	out_set_drv(XTRACER_DEFAULT_MODE_DRV);
}

void out_set_syn(enum XTRACER_MODE_SYN m)
{
	xtracer_out_env.syn = m;
}

void out_set_syn_intv(unsigned int v)
{
	xtracer_out_env.syn_intv = v;
}

void out_set_drv(enum XTRACER_MODE_DRV d)
{
	xtracer_out_env.drv = d;

	switch (d)
	{
		case XTRACER_DRV_SDL:
			xtracer_out_env.out_drv_init =  out_drv_sdl_init; 
			xtracer_out_env.out_drv_deinit =  out_drv_sdl_deinit;
			xtracer_out_env.out_drv_set =  out_drv_sdl_set;
			break;
		case XTRACER_DRV_IMG:
			/* Not implemented */
		case XTRACER_DRV_ASC:
			/* Not implemented */
		default:
			xtracer_out_env.out_drv_init = out_drv_dum_init; 
			xtracer_out_env.out_drv_deinit = out_drv_dum_deinit;
			xtracer_out_env.out_drv_set = out_drv_dum_set;
			break;
	}
}

int out_drv_init(unsigned int width, unsigned int height, unsigned int bpp)
{
	return (*xtracer_out_env.out_drv_init)(width, height, bbp);
}


int out_drv_deinit()
{
	return (*xtracer_out_env.out_drv_deinit)();
}


int out_drv_set(unsigned int x, unsigned int y, unsigned int color)
{
	return (*xtracer_out_env.out_drv_set)(x, y, color);
}

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */
