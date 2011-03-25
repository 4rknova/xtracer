/*

    This file is part of xtracer.

    Scene.cpp
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

#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nparse/parseutils.hpp>

#include "scene.hpp"

#include "cfgproto.h"

Scene::Scene(const char *filepath)
	: data(filepath), source(filepath)
{}

Scene::~Scene()
{
	if(!light.empty())
	{
		printf("Cleaning up..\n");
		printf("-> Lights..\n");
		for(std::list<Light *>::iterator it = light.begin(); it != light.end(); it++)
		{
			delete *it;
		}
	}
/*
	if(!material.empty())
	{
		printf("-> Materials..\n");
		for(std::list<Light *>::iterator it = material.begin(); it != material.end(); it++)
		{
			delete *it;
		}
	}
*/
	if(!geometry.empty())
	{
		printf("-> Geometry..\n");
		for(std::list<Geometry *>::iterator it = geometry.begin(); it != geometry.end(); it++)
		{
			delete *it;
		}
	}
}

xt_status_t Scene::init()
{
	printf ("Initiating the renderer\n");

	/* Load the data.file and parse it */
	int status = data.parse();
	if(status)
	{
		fprintf(stderr, "Error: Failed to load data.file.\n");
		return XT_STATUS_INVALID_SCENE_FILE;
	}

	analyze();

	/* Start populating the lists */
	unsigned int count = 0;
	/* Light sources */
	count = data.group(XT_CFGPROTO_NODE_LIGHT)->count_groups();
	if (count)
	{
		printf("Creating the light sources..\n");
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
		printf("Building the geometry..\n");
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
	printf("Analyzing data..\n");
	/* Print statistics */
	printf("-> Scene name: %s\n", data.get(XT_CFGPROTO_PROP_NAME));
	printf("-> Scene info: %s\n", data.get(XT_CFGPROTO_PROP_DESCRIPTION));

	std::string l;
	   
	data.group(XT_CFGPROTO_NODE_CAMERA)->list_groups(l);
	printf("-> Cameras: %i [%s]\n", data.group(XT_CFGPROTO_NODE_CAMERA)->count_groups(), l.c_str());
	data.group(XT_CFGPROTO_NODE_MATERIAL)->list_groups(l);
	printf("-> Materials: %i [%s]\n", data.group(XT_CFGPROTO_NODE_MATERIAL)->count_groups(), l.c_str());
	data.group(XT_CFGPROTO_NODE_LIGHT)->list_groups(l);
	printf("-> Lights: %i [%s]\n", data.group(XT_CFGPROTO_NODE_LIGHT)->count_groups(), l.c_str());
	data.group(XT_CFGPROTO_NODE_GEOMETRY)->list_groups(l);
	printf("-> Objects: %i [%s]\n", data.group(XT_CFGPROTO_NODE_GEOMETRY)->count_groups(), l.c_str());

	return XT_STATUS_OK;
}

xt_status_t Scene::set_camera(const char *name)
{
	/* Check if there are no cameras */
	if(data.group(XT_CFGPROTO_NODE_CAMERA)->count_groups() < 1)
	{
		printf("No cameras were specified. Nothing to render..\n");
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
			printf("No primary camera was specified..\n");
			dcam = data.group(XT_CFGPROTO_NODE_CAMERA)->group(1)->node();
		}   
	}

	/* Check if camera exists */
	if(!data.group(XT_CFGPROTO_NODE_CAMERA)->query_group(dcam.c_str()))
	{
		printf("Camera [ %s ] is invalid..\n", dcam.c_str());
		dcam = data.group(XT_CFGPROTO_NODE_CAMERA)->group(1)->node();
	}   

	printf("Using camera: %s\n", dcam.c_str());

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

	printf("-> FOV: %f\n", fov);
	printf("-> Position: %f %f %f\n", pos.x, pos.y, pos.z);
	printf("-> Target: %f %f %f\n", targ.x, targ.y, targ.z);
	printf("-> Up: %f %f %f\n", up.x, up.y, up.z);

	camera = Camera(pos, targ, up, fov);

	return XT_STATUS_OK;
}

xt_status_t Scene::add_light(NCFGParser *p)
{
	printf("-> Adding: %s\n", p->node().c_str());
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
	printf("-> Adding: %s\n", p->node().c_str());
	std::string type = p->get(XT_CFGPROTO_PROP_GEOMETRY);
	std::string stat;

	if (!type.compare(XT_CFGPROTO_VAL_SPHERE))
	{
		Geometry *tsphere = new Geometry(GEOMETRY_TYPE_SPHERE);

		std::string x,y,z;

		x = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
		y = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
		z = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);

		((Sphere *)(tsphere->get()))->origin = Vector3(nstring_to_double(x), nstring_to_double(y), nstring_to_double(z));
		printf("--> Origin: %f, %f, %f\n", ((Sphere *)(tsphere->get()))->origin.x, ((Sphere *)(tsphere->get()))->origin.y, ((Sphere *)(tsphere->get()))->origin.z);


		std::string radius = p->get(XT_CFGPROTO_PROP_RADIUS);
		((Sphere *)(tsphere->get()))->radius = nstring_to_double(radius);
		printf("--> Radius: %f\n", ((Sphere *)(tsphere->get()))->radius);

		geometry.push_back(tsphere);
	}
	else
	{
		stat = "[ Unsupported ]";
	}


	printf("--> Type: %s %s\n", type.c_str(), stat.c_str());

	return XT_STATUS_OK;
}

xt_status_t Scene::add_material(NCFGParser *p)
{
	return XT_STATUS_OK;
}
