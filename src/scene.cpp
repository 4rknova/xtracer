/*

    This file is part of xtracer.

    scene.cpp
    Scene class

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

#include <iostream>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nparse/parseutils.hpp>

#include "scene.hpp"

#include "cfgproto.h"

Scene::Scene(const char *filepath)
/* 
	The creation of the camera here serves as a precaution measure
	to avoid segmentation faults that would be caused by irregular 
	usage of this class. Normally it's not needed and we could just
	set the pointer to NULL.
*/
	: space(new SPScheme()), camera(new Camera()), data(filepath), source(filepath)
{}

Scene::~Scene()
{
	cleanup();
}

xt_status_t Scene::cleanup()
{
	/*
		Release the lights
	*/
	if(!light.empty())
	{
		std::cout << "Cleaning up..\n";
		std::cout << " Lights..\n";
		for(std::list<Light *>::iterator it = light.begin(); it != light.end(); it++)
		{
			delete *it;
		}
	}

	/*
		Release the materials
	*/
	if(!material.empty())
	{
		std::cout << " Materials..\n";
		for(std::list<Material *>::iterator it = material.begin(); it != material.end(); it++)
		{
			delete *it;
		}
	}

	/*
		Release the geometry
	*/
	if(!geometry.empty())
	{
		std::cout << " Geometry..\n";		
		for(std::list<Geometry *>::iterator it = geometry.begin(); it != geometry.end(); it++)
		{
			delete *it;
		}
	}

	/*
		Release the camera
	*/
	std::cout << " Camera..\n";
	delete camera;

	/*
		Release the spscheme
	*/
	delete space;
}

xt_status_t Scene::init()
{
	std::cout << "Initiating the scene..\n";		

	/* Load the scene file and parse it */
	int status = data.parse();
	if(status)
	{
		std::cerr << "Error: Failed to load the scene file.\n";
		return XT_STATUS_INVALID_SCENE_FILE;
	}

	analyze();

	/* Start populating the lists */
	unsigned int count = 0;
	/* Light sources */
	count = data.group(XT_CFGPROTO_NODE_LIGHT)->count_groups();
	if (count)
	{
		std::cout << "Creating the light sources..\n";
		for (unsigned int i = 1; i<= count; i++)
		{
			NCFGParser *lnode = data.group(XT_CFGPROTO_NODE_LIGHT)->group(i);
			add_light(lnode);
		}
	}

	/* Geometry */
	count = data.group(XT_CFGPROTO_NODE_GEOMETRY)->count_groups();
	if (count)
	{
		std::cout << "Building the geometry..\n";
		for (unsigned int i = 1; i<= count; i++)
		{
			NCFGParser *lnode = data.group(XT_CFGPROTO_NODE_GEOMETRY)->group(i);
			add_geometry(lnode);
		}
	}

	return XT_STATUS_OK;
}

xt_status_t Scene::analyze()
{
	std::cout << "Analyzing data..\n";
	/* Print statistics */
	std::cout << "Scene name: " << data.get(XT_CFGPROTO_PROP_NAME) << "\n";
	std::cout << "Scene info: " << data.get(XT_CFGPROTO_PROP_DESCRIPTION) << "\n";

	std::string l;
	int c = 0;
	   
	data.group(XT_CFGPROTO_NODE_CAMERA)->list_groups(l);
	c = data.group(XT_CFGPROTO_NODE_CAMERA)->count_groups();
	if (c)
		std::cout << "Cameras: " << c << " [ " << l << " ]\n";

	data.group(XT_CFGPROTO_NODE_MATERIAL)->list_groups(l);
	c = data.group(XT_CFGPROTO_NODE_MATERIAL)->count_groups();
	if (c)
		std::cout << "Materials: " << c << " [ " << l << " ]\n";

	data.group(XT_CFGPROTO_NODE_LIGHT)->list_groups(l);
	c = data.group(XT_CFGPROTO_NODE_LIGHT)->count_groups();
	if (c)
		std::cout << "Lights: " << c << " [ " << l << " ]\n";

	data.group(XT_CFGPROTO_NODE_GEOMETRY)->list_groups(l);
	c = data.group(XT_CFGPROTO_NODE_GEOMETRY)->count_groups();
	if (c)
		std::cout << "Objects: " << c << " [ " <<  l << " ]\n";

	return XT_STATUS_OK;
}

xt_status_t Scene::set_camera(const char *name)
{
	/* Check if there are no cameras */
	if(data.group(XT_CFGPROTO_NODE_CAMERA)->count_groups() < 1)
	{
		std::cout << "No cameras were specified. Nothing to render..\n";
		return XT_STATUS_NO_CAMERA;
	}

	/* Get the default camera */
	std::string dcam = name;

	if (dcam.empty())
	{
		/* Get the default camera */
		dcam = data.group(XT_CFGPROTO_NODE_CAMERA)->get(XT_CFGPROTO_PROP_DEFAULT);
		if (dcam.empty())
		{
			std::cout << "No primary camera was specified..\n";
			dcam = data.group(XT_CFGPROTO_NODE_CAMERA)->group(1)->node();
		}   
	}

	/* Check if camera exists */
	if(!data.group(XT_CFGPROTO_NODE_CAMERA)->query_group(dcam.c_str()))
	{
		std::cout << "Invalid camera: " << dcam << "\n";
		dcam = data.group(XT_CFGPROTO_NODE_CAMERA)->group(1)->node();
	}   

	std::cout << "Using camera: " << dcam << "\n";

	/* Create the camera */
	std::string f = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->get(XT_CFGPROTO_PROP_FOV);
	real_t fov = nstring_to_double(f);
	std::string x = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
	std::string y = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
	std::string z = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);

	Vector3 pos(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));
	
	x = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_TARGET)->get(XT_CFGPROTO_PROP_COORD_X);
	y = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_TARGET)->get(XT_CFGPROTO_PROP_COORD_Y);
	z = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_TARGET)->get(XT_CFGPROTO_PROP_COORD_Z);

	Vector3 targ(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));

	x = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_UP)->get(XT_CFGPROTO_PROP_COORD_X);
	y = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_UP)->get(XT_CFGPROTO_PROP_COORD_Y);
	z = data.group(XT_CFGPROTO_NODE_CAMERA)->group(dcam.c_str())->group(XT_CFGPROTO_PROP_COORD_UP)->get(XT_CFGPROTO_PROP_COORD_Z);

	Vector3 up(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));

	std::cout << "     FOV: " << fov  << "\n";
	std::cout << "Position: " << pos  << "\n";
	std::cout << "  Target: " << targ << "\n";
	std::cout << "      Up: " << up   << "\n";

	delete camera;

	camera = new Camera(pos, targ, up, fov);

	return XT_STATUS_OK;
}

xt_status_t Scene::add_light(NCFGParser *p)
{
	std::cout << "Adding: " << p->node() << "\n";
	std::string x = p->group(XT_CFGPROTO_PROP_COORD_TARGET)->get(XT_CFGPROTO_PROP_COORD_X);
	std::string y = p->group(XT_CFGPROTO_PROP_COORD_TARGET)->get(XT_CFGPROTO_PROP_COORD_Y);
	std::string z = p->group(XT_CFGPROTO_PROP_COORD_TARGET)->get(XT_CFGPROTO_PROP_COORD_Z);

	Vector3 position(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));
	Light *tlight = new Light(position);
	light.push_back(tlight);

	return XT_STATUS_OK;
}

#include <nmath/sphere.h>

xt_status_t Scene::add_geometry(NCFGParser *p)
{
	std::cout << "Adding: " << p->node() << "\n";
	std::string type = p->get(XT_CFGPROTO_PROP_GEOMETRY);

	std::cout << "  Type: " << type << "\n";

	if (!type.compare(XT_CFGPROTO_VAL_SPHERE))
	{
		Geometry *tsphere = new Geometry(GEOMETRY_TYPE_SPHERE);

		std::string x,y,z;

		x = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
		y = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
		z = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);

		((Sphere *)(tsphere->get()))->origin = Vector3(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));
		std::cout << "Origin: " << ((Sphere *)(tsphere->get()))->origin << "\n";


		std::string radius = p->get(XT_CFGPROTO_PROP_RADIUS);
		((Sphere *)(tsphere->get()))->radius = nstring_to_double(radius);
		std::cout << "Radius: " << ((Sphere *)(tsphere->get()))->radius << "\n";

		geometry.push_back(tsphere);
	}
	else
	{
		std::cerr << "Warning: Unsupported type. Skipping...\n";
	}

	return XT_STATUS_OK;
}

xt_status_t Scene::add_material(NCFGParser *p)
{
	return XT_STATUS_OK;
}