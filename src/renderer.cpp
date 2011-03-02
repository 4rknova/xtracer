/*

	This file is part of xtracer.

	renderer.cpp
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

#include <cstdio>
#include <nparse/cfgparser.hpp>

#include "renderer.hpp"

Xtracer_renderer_env_t xtracer_renderer_env;

void xtrenderer_init()
{
	xtracer_renderer_env.width = XTRACER_DEFAULT_SCREEN_WIDTH;
	xtracer_renderer_env.height = XTRACER_DEFAULT_SCREEN_HEIGHT;
	xtracer_renderer_env.rdepth = XTRACER_DEFAULT_RECURSION_DEPTH;
}

void xtrenderer_set_width(unsigned int v)
{
	xtracer_renderer_env.width = v;
}

void xtrenderer_set_height(unsigned int v)
{
	xtracer_renderer_env.height = v;
}

void xtrenderer_set_rdepth(unsigned int v)
{
	xtracer_renderer_env.rdepth = v;
}

unsigned int xtrenderer_get_width()
{
	return xtracer_renderer_env.width;
}

unsigned int xtrenderer_get_height()
{
	return xtracer_renderer_env.height;
}

unsigned int xtrenderer_get_rdepth()
{
	return xtracer_renderer_env.rdepth;
}

xt_status_t xtrender(const char* scenefile)
{
	XTFramebuffer fb(xtracer_renderer_env.width, xtracer_renderer_env.height);

	/* Load the scene file and parse it. */
	printf("Analyzing scene..\n");
	NCFGParser scene(scenefile);
	scene.parse();

//	scene->group("camera")

	printf("Rendering frame..\n");
	return XTRACER_STATUS_OK;
}
