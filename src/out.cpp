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

#include <cstdio>

#include "out.hpp"

#include "err.h"

Xtracer_out_env_t xtracer_out_env;

void out_init()
{
	xtracer_out_env.syn = XTRACER_DEFAULT_MODE_SYN;
	xtracer_out_env.syn_intv = XTRACER_DEFAULT_ASYN_INTV;
	xtracer_out_env.drv = XTRACER_DEFAULT_MODE_DRV;
}

void out_set_syn(enum XTRACER_MODE_SYN m)
{
	xtracer_out_env.syn = m;
}

void out_set_syn_intv(unsigned int v)
{
	xtracer_out_env.syn_intv = v;
}

void out_set_drv(XTRACER_MODE_DRV d)
{
	xtracer_out_env.drv = d;
}
