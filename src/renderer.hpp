/*

	This file is part of xtracer.

	renderer.hpp
	Renderer

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

#ifndef XTRACER_RENDERER_HPP_INCLUDED
#define XTRACER_RENDERER_HPP_INCLUDED

#include "err.h"
#include "fb.hpp"

/* Default values for the screen buffer dimensions */
#define XTRACER_DEFAULT_SCREEN_WIDTH    640
#define XTRACER_DEFAULT_SCREEN_HEIGHT   480

/* Default values for the raytracer environment */
#define XTRACER_DEFAULT_RECURSION_DEPTH 5

/* Output environment */
struct Xtracer_renderer_env_t
{
	unsigned int width;		/* Output buffer width */
	unsigned int height;	/* Output buffer height */
	unsigned int rdepth;	/* Maximum recursion depth */
};

void xtrenderer_init();									/* Initiate the renderer */

void xtrenderer_set_width(unsigned int v);				/* Set framebuffer width */
void xtrenderer_set_height(unsigned int v);				/* Set framebuffer height */
void xtrenderer_set_rdepth(unsigned int v);				/* Set the recursion depth */

unsigned int xtrenderer_get_width();					/* Get framebuffer width */
unsigned int xtrenderer_get_height();					/* Get framebuffer height */
unsigned int xtrenderer_get_rdepth();					/* Get framebuffer rdepth */

xt_status_t xtrender(const char* scenefile);			/* Render the scene */

#endif /* XTRACER_RENDERER_HPP_INCLUDED */
