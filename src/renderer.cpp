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
	/* Load the scene file and parse it. */
	printf("Analyzing scene..\n");
	NCFGParser scene(scenefile);
	int status = scene.parse();

	if(status)
	{
		fprintf(stderr, "Error: Failed to load scene file.\n");
		return XTRACER_STATUS_INVALID_SCENE_FILE;
	}

	std::string l;

	printf("  Scene name: %s\n", scene.get("name"));
	printf("  Scene info: %s\n", scene.get("description"));

	scene.group("camera")->list_groups(l);
	printf("     Cameras: %i [%s]\n", scene.group("camera")->count_groups(), l.c_str());
	scene.group("material")->list_groups(l);
	printf("   Materials: %i [%s]\n", scene.group("material")->count_groups(), l.c_str());
	scene.group("light")->list_groups(l);
	printf("      Lights: %i [%s]\n", scene.group("light")->count_groups(), l.c_str());
	scene.group("object")->list_groups(l);
	printf("     Objects: %i [%s]\n", scene.group("object")->count_groups(), l.c_str());

	printf("Rendering frame..\n");

	XTFramebuffer fb(xtracer_renderer_env.width, xtracer_renderer_env.height);





	return XTRACER_STATUS_OK;
}
