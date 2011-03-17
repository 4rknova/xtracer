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

Renderer::Renderer(Framebuffer *fb, unsigned int depth):
	m_p_fb(fb), m_p_depth(depth)
{}

unsigned int Renderer::recursion_depth()
{
	return m_p_depth;
}

unsigned int Renderer::set_recursion_depth(unsigned int depth)
{
	return m_p_depth = depth;
}

#include <stdint.h>

xt_status_t Renderer::render(const char* scenefile)
{
	if(m_p_fb == NULL)
	{
		fprintf(stderr, "Error: Invalid framebuffer (NULL).\n");
		return XTRACER_STATUS_INVALID_FB;
	}

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

	/* Rendering loop */
	for (unsigned int y = 0; y < m_p_fb->height(); y++)
	{
		for (unsigned int x = 0; x < m_p_fb->width(); x++)
		{
		//	uint32_t final_color = 0;
			// compute first ray
			// For each object, determine first intersection
			// Check if in shadow
		}
	}

	return XTRACER_STATUS_OK;
}
