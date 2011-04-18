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
#include <iomanip>

#include <nmath/vector.h>
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
	: space(new SPScheme()), camera(new Camera()), 
	source(filepath), data(filepath), 
	rdepth(1),
	/* 
		RGB -> RGBA value
	*/
	background_color(0)
{}

Scene::~Scene()
{
	cleanup();
}

void Scene::cleanup()
{
	std::cout << "Cleaning up..\n";

	/*
		Release the lights
	*/
	if(!light.empty())
	{
		std::cout << "Releasing the lights..\n";
		for(std::map<std::string, Light *>::iterator it = light.begin(); it != light.end(); it++)
		{
			delete (*it).second;
		}
	}

	/*
		Release the materials
	*/
	if(!material.empty())
	{
		std::cout << "Releasing the materials..\n";
		for(std::map<std::string, Material *>::iterator it = material.begin(); it != material.end(); it++)
		{
			delete (*it).second;
		}
	} 

	/*
		Release the geometry
	*/
	if(!geometry.empty())
	{
		std::cout << "Releasing the geometry..\n";
		for(std::map<std::string, Geometry *>::iterator it = geometry.begin(); it != geometry.end(); it++)
		{
			delete (*it).second;
		}
	} 

	/*
		Release the camera
	*/
	std::cout << "Releasing the camera..\n";
	delete camera;

	/*
		Release the spscheme
	*/
	std::cout << "\rReleasing the space partitioning scheme..\n";
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
	std::cout << "Generating the scene..\n";

	/* Start populating the lists */
	unsigned int count = 0;

	/*
		BACKGROUND COLOR
	*/
	set_bgcolor();

	/* 
		Light sources 
	*/
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

	/*
		Materials
	*/
	count = data.group(XT_CFGPROTO_NODE_MATERIAL)->count_groups();
	if(count)
	{
		std::cout << "Creating the materials..\n";
		for (unsigned int i = 1; i <= count; i++)
		{
			NCFGParser *lnode = data.group(XT_CFGPROTO_NODE_MATERIAL)->group(i);
			add_material(lnode);
		}
	}
	
	/* 
		Geometry 
	*/
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

	/*
		Initiate the space partitioning scheme here
	*/

	space->build(geometry);

	return XT_STATUS_OK;
}

xt_status_t Scene::analyze()
{
	std::cout << "Analyzing scene data..\n";
	/* Print statistics */
	std::string name = data.get(XT_CFGPROTO_PROP_NAME);
	std::string info = data.get(XT_CFGPROTO_PROP_DESCRIPTION);

	if(!name.empty())
		std::cout << "Scene name: " << data.get(XT_CFGPROTO_PROP_NAME) << "\n";
	if(!info.empty())
		std::cout << "Scene info: " << data.get(XT_CFGPROTO_PROP_DESCRIPTION) << "\n";

	std::string l;
	int c = 0;

	background_color = 255; /* Alpha channel */

	c = data.group(XT_CFGPROTO_NODE_CAMERA)->count_groups();
	if (c)
	{
		data.group(XT_CFGPROTO_NODE_CAMERA)->list_groups(l);
		std::cout << "Listing cameras  :" << " [" << std::setw(4) << c << " ] " << l << "\n";
	}

	c = data.group(XT_CFGPROTO_NODE_MATERIAL)->count_groups();
	if (c)
	{
		data.group(XT_CFGPROTO_NODE_MATERIAL)->list_groups(l);
		std::cout << "Listing materials:" << " [" << std::setw(4) << c << " ] " << l << "\n";
	}

	c = data.group(XT_CFGPROTO_NODE_LIGHT)->count_groups();
	if (c)
	{
		data.group(XT_CFGPROTO_NODE_LIGHT)->list_groups(l);
		std::cout << "Listing lights   :" << " [" << std::setw(4) << c << " ] " << l << "\n";
	}

	c = data.group(XT_CFGPROTO_NODE_GEOMETRY)->count_groups();
	if (c)
	{
		data.group(XT_CFGPROTO_NODE_GEOMETRY)->list_groups(l);
		std::cout << "Listing objects  :" << " [" << std::setw(4) << c << " ] " << l << "\n";
	}

	return XT_STATUS_OK;
}

xt_status_t Scene::set_bgcolor()
{
	float r = 0, g = 0, b = 0, a = 255;
	std::string v;
	v = data.group(XT_CFGPROTO_PROP_BGCOLOR)->get(XT_CFGPROTO_PROP_COLOR_R);
	r = nstring_to_double(v) * 255;
	v = data.group(XT_CFGPROTO_PROP_BGCOLOR)->get(XT_CFGPROTO_PROP_COLOR_G);
	g = nstring_to_double(v) * 255;
	v = data.group(XT_CFGPROTO_PROP_BGCOLOR)->get(XT_CFGPROTO_PROP_COLOR_B);
	b = nstring_to_double(v) * 255;

	background_color = rgba_to_pixel32((int)r, (int)g, (int)b, a);

	std::cout 
		<< "Background color: 0x" << std::hex << std::setfill('0') 
		<< std::setw(8) << background_color 
		<< std::dec << "\n";

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
/*
	std::cout << "FOV: " << fov << "\n";
	std::cout << "Position: " << pos << "\n";
	std::cout << "Target: " << targ << "\n";
	std::cout << "Up: " << up << "\n";
*/
	delete camera;

	camera = new Camera(pos, targ, up, fov);

	return XT_STATUS_OK;
}

xt_status_t Scene::add_light(NCFGParser *p)
{
	std::cout << "Adding: " << p->node() << "\n";
	/* Retrieve the properties */
	/* Position */
	std::string posx = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
	std::string posy = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
	std::string posz = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);

	/* Intensity */
	std::string colr = p->group(XT_CFGPROTO_PROP_INTENSITY)->get(XT_CFGPROTO_PROP_COLOR_R);
	std::string colg = p->group(XT_CFGPROTO_PROP_INTENSITY)->get(XT_CFGPROTO_PROP_COLOR_G);
	std::string colb = p->group(XT_CFGPROTO_PROP_INTENSITY)->get(XT_CFGPROTO_PROP_COLOR_B);

	/* Create the light */
	Light *tlight = new Light();

	tlight->position = 
		Vector3(nstring_to_double(posx), 
				nstring_to_double(posy), 
				nstring_to_double(posz));
	tlight->intensity = 
		Vector3(nstring_to_double(colr), 
				nstring_to_double(colg), 
				nstring_to_double(colb));

//	std::cout << tlight->position.z << "\n\n\n"; getchar();
/*
	std::cout << "Position: " << tlight->position << "\n";
	std::cout << "Intensity: " << tlight->intensity << "\n";
*/
	/* Add to the lights list */
	light[p->node()] = tlight;

	return XT_STATUS_OK;
}

#include <nmath/sphere.h>

xt_status_t Scene::add_geometry(NCFGParser *p)
{
	std::string type = p->get(XT_CFGPROTO_PROP_TYPE);

	std::cout << "Adding " << type << ": " << p->node() << "\n";

	Geometry *tobj = NULL;

	if (!type.compare(XT_CFGPROTO_VAL_SPHERE))
 	{
		tobj = new Geometry(GEOMETRY_TYPE_SPHERE);

		std::string x, y, z, r;

		x = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_X);
		y = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Y);
		z = p->group(XT_CFGPROTO_PROP_COORD_POSITION)->get(XT_CFGPROTO_PROP_COORD_Z);
		r = p->get(XT_CFGPROTO_PROP_RADIUS);

		((Sphere *)(tobj->get()))->origin = Vector3(
			nstring_to_double(x), 
			nstring_to_double(y), 
			nstring_to_double(z));

		((Sphere *)(tobj->get()))->radius = nstring_to_double(r);
/*
		std::cout << "Origin: " << ((Sphere *)(tobj->get()))->origin << "\n";
		std::cout << "Radius: " << ((Sphere *)(tobj->get()))->radius << "\n";
*/
		geometry[p->node()] = tobj;
	}
	else
	{
		std::cout << "Warning: Unsupported type. Skipping...\n";
	}

	/* Set the material */
	if(tobj)
	{
		tobj->material = std::string(p->get(XT_CFGPROTO_PROP_MATERIAL));
	}

	return XT_STATUS_OK;
} 

#include "matlambert.hpp"

xt_status_t Scene::add_material(NCFGParser *p)
{
	std::string type = p->get(XT_CFGPROTO_PROP_TYPE);
	
	std::cout << "Adding " << type << " material: " << p->node() << "\n";

	if (!type.compare(XT_CFGPROTO_VAL_LAMBERT))
	{
		std::string colr = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_R);
		std::string colg = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_G);
		std::string colb = p->group(XT_CFGPROTO_PROP_DIFFUSE)->get(XT_CFGPROTO_PROP_COLOR_B);

		std::string refl = p->get(XT_CFGPROTO_PROP_REFLECTANCE);
		
		Material *tmat = new Material(MATERIAL_TYPE_LAMBERT);
	
		((MatLambert *)(tmat->get()))->diffuse = 
			Vector3(nstring_to_double(colr), 
					nstring_to_double(colg), 
					nstring_to_double(colb));

		((MatLambert *)(tmat->get()))->reflectance = nstring_to_double(refl);

		material[p->node()] = tmat;
	}
	else
	{
		std::cout << "Warning: Unsupported model. Skipping...\n";
	}

	return XT_STATUS_OK;
}
