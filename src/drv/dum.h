/*

	This file is part of xtracer.

	dum.h
	Dummy driver

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

#ifndef XTRACER_DRV_DUM_H_INCLUDED
#define XTRACER_DRV_DUM_H_INCLUDED

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

int out_drv_dum_init();
int out_drv_dum_deinit();

int out_drv_dum_set(unsigned int x, unsigned int y, unsigned int color);

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */

#endif /* XTRACER_DRV_DUM_H_INCLUDED */
