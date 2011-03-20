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
#include <iostream>
#include <nparse/cfgparser.hpp>
#include <nparse/parseutils.hpp>
#include <nmath/vector.h>
#include <nmath/ray.h>

#include "renderer.hpp"
#include "camera.hpp"
#include "light.hpp"


#define XT_NODE_CAMERA			"camera"
#define XT_NODE_LIGHT			"light"
#define XT_NODE_MATERIAL		"material"
#define XT_NODE_GEOMETRY		"object"

#define XT_PROP_DEFAULT 		"default"

#define XT_PROP_FOV				"fov"

#define XT_PROP_COORD_POSITION	"position"
#define XT_PROP_COORD_TARGET	"target"
#define XT_PROP_COORD_UP		"up"


#define XT_PROP_COORD_X			"ix"
#define XT_PROP_COORD_Y			"iy"
#define XT_PROP_COORD_Z			"iz"


Renderer::Renderer(Framebuffer &fb, unsigned int depth):
	m_p_fb(&fb), m_p_depth(depth)
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

#include "camera.hpp"

xt_status_t Renderer::render(const char* scenefile, const char *camera)
{
	if(m_p_fb == NULL)
	{
		fprintf(stderr, "Error: Invalid framebuffer (NULL).\n");
		return XT_STATUS_INVALID_FB;
	}

	/* Load the scene file and parse it. */
	printf("Analyzing scene..\n");
	NCFGParser scene(scenefile);
	int status = scene.parse();

	if(status)
	{
		fprintf(stderr, "Error: Failed to load scene file.\n");
		return XT_STATUS_INVALID_SCENE_FILE;
	}

	std::string l;

	/* Print statistics */
	printf("-> Scene name: %s\n", scene.get("name"));
	printf("-> Scene info: %s\n", scene.get("description"));

	scene.group(XT_NODE_CAMERA)->list_groups(l);
	printf("-> Cameras: %i [%s]\n", scene.group(XT_NODE_CAMERA)->count_groups(), l.c_str());
	scene.group(XT_NODE_MATERIAL)->list_groups(l);
	printf("-> Materials: %i [%s]\n", scene.group(XT_NODE_MATERIAL)->count_groups(), l.c_str());
	scene.group(XT_NODE_LIGHT)->list_groups(l);
	printf("-> Lights: %i [%s]\n", scene.group(XT_NODE_LIGHT)->count_groups(), l.c_str());
	scene.group(XT_NODE_GEOMETRY)->list_groups(l);
	printf("-> Objects: %i [%s]\n", scene.group(XT_NODE_GEOMETRY)->count_groups(), l.c_str());

	/* Check if there are no cameras */
	if(scene.group(XT_NODE_CAMERA)->count_groups() < 1)
	{
		printf("No cameras were specified. Nothing to render..\n");
		return XT_STATUS_NO_CAMERA;
	}

	/* Get the default camera */
	std::string dcam = camera; 
	
	if (dcam.empty())
	{

		/* Get the default camera */
		dcam = scene.group(XT_NODE_CAMERA)->get(XT_PROP_DEFAULT);

		if (dcam.empty())
		{
			printf("No primary camera was specified..\n");
			dcam = scene.group(XT_NODE_CAMERA)->group(1)->node();
		}
	}

	/* Check if camera exists */

	if(!scene.group(XT_NODE_CAMERA)->query_group(dcam.c_str()))
	{
		printf("Camera [ %s ] is invalid..\n", dcam.c_str());
		dcam = scene.group(XT_NODE_CAMERA)->group(1)->node();
	}
	
	printf("Using camera: %s\n", dcam.c_str());

	/* Create the camera */
	std::string f = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->get(XT_PROP_FOV);
	real_t fov = nstring_to_double(f);

	std::string x = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_POSITION)->get(XT_PROP_COORD_X);
	std::string y = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_POSITION)->get(XT_PROP_COORD_Y);
	std::string z = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_POSITION)->get(XT_PROP_COORD_Z);

	Vector3 pos(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));

	x = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_TARGET)->get(XT_PROP_COORD_X);
	y = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_TARGET)->get(XT_PROP_COORD_Y);
	z = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_TARGET)->get(XT_PROP_COORD_Z);

	Vector3 targ(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));

	x = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_UP)->get(XT_PROP_COORD_X);
	y = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_UP)->get(XT_PROP_COORD_Y);
	z = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_UP)->get(XT_PROP_COORD_Z);

	Vector3 up(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));

	printf("-> FOV: %f\n", fov);
	printf("-> Position: %f %f %f\n", pos.x, pos.y, pos.z);
	printf("-> Target: %f %f %f\n", targ.x, targ.y, targ.z);
	printf("-> Up: %f %f %f\n", up.x, up.y, up.z);

	Camera cam(pos, targ, up, fov);

	/* Prepare the scene graph */
	printf("Preparing the scene..\n");
	/* Light sources */
	printf("Lights..\n");
	std::list<Light> light;

	for (unsigned int i = 1; i< scene.group(XT_NODE_LIGHT)->count_groups(); i++)
	{
		std::string lnode = scene.group(XT_NODE_LIGHT)->group(i)->node();
		printf("Processing light: %s\n", lnode.c_str());
		x = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_TARGET)->get(XT_PROP_COORD_X);
		y = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_TARGET)->get(XT_PROP_COORD_Y);
		z = scene.group(XT_NODE_CAMERA)->group(dcam.c_str())->group(XT_PROP_COORD_TARGET)->get(XT_PROP_COORD_Z);

		pos.x = nstring_to_double(x);
		pos.y = nstring_to_double(y);
		pos.z = nstring_to_double(z);

		Light tlight(pos);

		light.push_back(tlight);

	}

	/* Geometry */

	/* Materials */


	/* Render */
	printf("Rendering frame..\n");

	/* Rendering loop */
	float total_pixels =  m_p_fb->height() *  m_p_fb->width();
	for (unsigned int h = 0; h < m_p_fb->height(); h++)
	{
		for (unsigned int w = 0; w < m_p_fb->width(); w++)
		{
			Ray primary = cam.get_primary_ray(w, h, m_p_fb->width(), m_p_fb->height());


		//	uint32_t final_color = 0;
			// compute first ray
			// For each object, determine first intersection
			// Check if in shadow
		
			printf("\rProgress: %06.2f%% of %i pixels.", ( (h * m_p_fb->width() + w)  / (total_pixels - 1)) * 100, (int)total_pixels);
			std::cout << std::flush;
		}
	}
	printf("\n");

	return XT_STATUS_OK;
}
